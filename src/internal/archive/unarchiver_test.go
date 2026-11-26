package archive

import (
	"archive/tar"
	"compress/gzip"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"testing"

	"github.com/stretchr/testify/require"
)

/*
table tests
- simple archive with one log file
- empty archive
- archive with big files > 2MB
*/
func Test_Table_UnpackArchive_Successful(t *testing.T) {

	cases := []struct {
		TestCaseTitle   string
		FileAndLines    map[string]int
		FileNumExpected int
	}{
		{
			"Simple case, several small log files",
			map[string]int{
				"logs/app.log":      2,
				"logs/nested/b.log": 1,
			}, 2},
		{
			"Case with big log file",
			map[string]int{
				//150000 строк > 2МБ
				//150000 lines > 2MB
				"logs/app.log": 150000,
			}, 1,
		},
		{
			"Case with empty archive",
			map[string]int{}, 0,
		},
	}

	for _, testCase := range cases {
		t.Run(testCase.TestCaseTitle,
			func(t *testing.T) {
				archivePath := makeTarGz(t, testCase.FileAndLines)

				tempDir, files, err := UnpackArchive(archivePath)
				require.NoError(t, err)
				defer func(t *testing.T) {
					err := os.RemoveAll(tempDir)
					require.NoError(t, err)
				}(t)
				require.Equal(t, testCase.FileNumExpected, len(files))

				for _, fileName := range files {
					f, err := os.Open(fileName)
					require.NoError(t, err)
					err = f.Close()
					require.NoError(t, err)
				}
			})
	}

}

/*
here used random invalid path
*/
func Test_UnpackArchive_Err_NoArchive(t *testing.T) {
	archivePath := t.TempDir() + "no_such_archive.tar.gz"

	_, err := os.Open(archivePath)
	if err == nil {
		err = os.Remove(archivePath)
		require.NoError(t, err)
	}
	_, _, err = UnpackArchive(archivePath)
	require.Error(t, err)
}

const (
	//максимальное количество строк, которое записывается за один вызов Write
	//max number of lines written per single Write call
	maxLinesPerWrite = 1000
)

// шаблоны строк логов
// log line templates
var logLinePatterns = []string{
	"INFO ok %d\n",
	"ERROR boom %d\n",
	"WARN slow %d\n",
}

// help func for creating temp archives
// files: file name in archive -> number of log lines
func makeTarGz(t *testing.T, files map[string]int) string {
	t.Helper()

	path := filepath.Join(t.TempDir(), "test.tar.gz")
	out, err := os.Create(path)
	require.NoError(t, err)
	defer out.Close()

	gw := gzip.NewWriter(out)
	defer gw.Close()

	tw := tar.NewWriter(gw)
	defer tw.Close()

	for name, lineNum := range files {
		chunks := makeContentChunks(lineNum)

		size := 0
		for _, chunk := range chunks {
			size += len(chunk)
		}

		hdr := &tar.Header{
			Name: name,
			Mode: 0o644,
			Size: int64(size),
		}
		require.NoError(t, tw.WriteHeader(hdr))

		for _, chunk := range chunks {
			_, err := tw.Write([]byte(chunk))
			require.NoError(t, err)
		}
	}

	return path
}

// генерирует lineNum строк по шаблонам logLinePatterns,
// разбивая их на части по maxLinesPerWrite строк
// generates lineNum lines from logLinePatterns,
// split into chunks of maxLinesPerWrite lines
func makeContentChunks(lineNum int) []string {
	chunks := make([]string, 0, lineNum/maxLinesPerWrite+1)
	var sb strings.Builder

	for start := 0; start < lineNum; start += maxLinesPerWrite {
		end := min(start+maxLinesPerWrite, lineNum)

		sb.Reset()
		for i := start; i < end; i++ {
			sb.WriteString(fmt.Sprintf(logLinePatterns[i%len(logLinePatterns)], i))
		}
		chunks = append(chunks, sb.String())
	}

	return chunks
}
