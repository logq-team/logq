package archive

import (
	"archive/tar"
	"compress/gzip"
	"errors"
	"io"
	"os"
	"path/filepath"
)

const (
	DestPath = "logq_archive"
	//буфер размером 2МБ
	//size of buffer is 2MB
	BufferSizeInBytes = 1024 * 1024 * 2
)

var (
	ErrInvalidPath = errors.New("Invalid path")
)

func UnpackArchive(archiveName string) ([]string, error) {
	filesNames := make([]string, 0, 4)
	file, err := os.Open(archiveName)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	gzr, err := gzip.NewReader(file)
	if err != nil {
		return nil, err
	}
	defer gzr.Close()

	tr := tar.NewReader(gzr)

	for {
		header, err := tr.Next()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}

		targetPathRes, err := processTarHeader(DestPath, header, tr)
		if err != nil {
			return nil, err
		}

		if targetPathRes != nil {
			filesNames = append(filesNames, *targetPathRes)
		}
	}

	return filesNames, nil
}

func processTarHeader(destPath string,
	header *tar.Header,
	tr *tar.Reader) (*string, error) {
	var err error
	targetPath := filepath.Join(destPath, header.Name)
	var targetPathRes *string

	//fmt.Println("Тип файла ", header.Typeflag)

	switch header.Typeflag {
	case tar.TypeDir:
		if validationRes := ValidatePath(targetPath); !validationRes {
			return nil, ErrInvalidPath
		}
		if err = os.MkdirAll(targetPath, os.FileMode(header.Mode)); err != nil {
			return nil, err
		}
	case tar.TypeReg:
		if validationRes := ValidatePath(targetPath); !validationRes {
			return nil, ErrInvalidPath
		}
		if err = os.MkdirAll(filepath.Dir(targetPath), 0755); err != nil {
			return nil, err
		}

		if err = writeTarFile(targetPath, tr, os.FileMode(header.Mode)); err != nil {
			return nil, err
		}

		targetPathRes = &targetPath
	default:
		//fmt.Printf("Типы кроме директорий и обычных файлов пропускаются\n")
	}

	return targetPathRes, nil
}

func writeTarFile(targetPath string, tr *tar.Reader, mode os.FileMode) error {
	outFile, err := os.OpenFile(targetPath, os.O_CREATE|os.O_RDWR|os.O_TRUNC, mode)
	if err != nil {
		return err
	}
	defer outFile.Close()

	buffer := make([]byte, BufferSizeInBytes)
	for {
		nIn, err := tr.Read(buffer)
		if nIn > 0 {
			if _, err := outFile.Write(buffer[:nIn]); err != nil {
				return err
			}
		} else if err != nil && err != io.EOF {
			return err
		} else if err == io.EOF {
			break
		}
	}

	return nil
}
