package engine

import (
	"fmt"
	"logq/src/cppengine"
	"logq/src/internal/archive"
	"os"
	"runtime"
	"strings"
	"time"
)

type FileResult struct {
	Path        string
	SizeInBytes uint64
	Lines       uint64
	ErrNum      uint64
	WarnNum     uint64
	Status      cppengine.FileStatus
}

// for prepareStringFileResults
type rowData []string

// первый возвращаемый тип - string
// это вывод результата в случае успешного завершения операции
// второй - error
// будет содержать вывод в случае ошибки
func HandleOverview(archiveName string) (string, error) {
	tempDir, fileNames, err := archive.UnpackArchive(archiveName)
	defer func() {
		errIn := os.RemoveAll(tempDir)
		if errIn != nil {
			fmt.Printf("HandleOverview:\ncall os.RemoveAll: %s\n", errIn.Error())
		}
	}()
	// используем значение от райнтама
	workerThreadNum := runtime.GOMAXPROCS(0)

	if err != nil {
		return "", err
	}

	start := time.Now()
	stats, err := cppengine.OverviewCall(fileNames, workerThreadNum)
	seconds := time.Since(start).Seconds()

	if err != nil {
		return "", err
	}

	//TODO добавить --all
	return overviewOutput(
		tempDir, fileNames, stats, workerThreadNum, seconds, 5), nil
}

func overviewOutput(rootDir string,
	fileNames []string,
	results []cppengine.FileStats,
	workerThreadsNum int,
	timeForScanningInSec float64,
	maxFileResultsForOutput int) string {
	var outputResB strings.Builder

	//redacting words "file/files" and "thread/threads"
	fileWord := "file"
	if len(fileNames) > 1 {
		fileWord = "files"
	}

	threadWord := "thread"
	if workerThreadsNum > 1 {
		threadWord = "threads"
	}

	//counting size of all files
	var sizeInBytes uint64
	for _, res := range results {
		sizeInBytes += res.Bytes
	}

	//scanned n files
	outputResB.WriteString(fmt.Sprintf("scanned %d %s, %s in %.1f s (%d %s)\n\n",
		len(fileNames), fileWord, prepareSize(sizeInBytes), timeForScanningInSec, workerThreadsNum, threadWord))

	fileResNumForOutput := min(maxFileResultsForOutput, len(results))
	if maxFileResultsForOutput == -1 {
		fileResNumForOutput = len(results)
	}

	//first sorting results

	fileResultsForOutput := make([]FileResult, fileResNumForOutput)
	for i := 0; i < fileResNumForOutput; i++ {
		fName := fileNames[i]
		srcPath := fName[len(rootDir)+1:]

		fileResultsForOutput[i] = FileResult{
			Path:        srcPath,
			SizeInBytes: results[i].Bytes,
			Lines:       results[i].Lines,
			ErrNum:      results[i].ErrorLines,
			WarnNum:     results[i].WarnLines,
			Status:      results[i].Status,
		}
	}

	outputResB.WriteString(prepareStringFileResults(fileResultsForOutput))
	if len(results) > fileResNumForOutput {
		outputResB.WriteString(fmt.Sprintf("... %d more files results", (len(results) - fileResNumForOutput)))
	}

	return outputResB.String()
}

/*
заготавливает вывод результатов в формате небольшой таблички:
FILE                           SIZE    LINES   ERROR   WARN    STATUS
integration-tests/runner.log   3.9 GB  41.2M   312     1840    OK
services/payments.log          1.6 GB  17.0M   57      902     OK
services/gateway.log.gz        410 MB  5.3M    9       44      OK
build/compile.log              12 MB   140K    0       3       OK
broken-file.log                -       -       -       -       OPEN_ERROR
*/
func prepareStringFileResults(fileResults []FileResult) string {
	headers := []string{"FILE", "SIZE", "LINES", "ERROR", "WARN", "STATUS"}

	// 1. Подготавливаем данные для каждой строки
	var rows []rowData

	for _, r := range fileResults {
		statusStr := statusToString(r.Status)

		sizeStr := "-"
		linesStr := "-"
		errStr := "-"
		warnStr := "-"

		// Если статус OK, показываем метрики, иначе везде "-"
		if r.Status == cppengine.FileOK {
			sizeStr = prepareSize(r.SizeInBytes)
			linesStr = prepareLines(r.Lines)
			errStr = fmt.Sprintf("%d", r.ErrNum)
			warnStr = fmt.Sprintf("%d", r.WarnNum)
		}

		rows = append(rows, rowData{r.Path, sizeStr, linesStr, errStr, warnStr, statusStr})
	}

	// 2. Вычисляем максимальную ширину для каждого столбца
	widths := make([]int, len(headers))
	for i, h := range headers {
		widths[i] = len(h) // Изначально ширина = длине заголовка
	}

	for _, row := range rows {
		for i, val := range row {
			if len(val) > widths[i] {
				widths[i] = len(val)
			}
		}
	}

	// 3. Применяем правило минимальной ширины
	for i := range widths {
		if i == 0 {
			// Для колонки FILE: если самое длинное значение не очень большое, ставим 30
			if widths[i] < 30 {
				widths[i] = 30
			}
		} else {
			// Для остальных колонок можно сделать минимальную ширину поменьше (например, 8)
			if widths[i] < 8 {
				widths[i] = 8
			}
		}
		widths[i] += 2 // Добавляем 2 пробела для отступа между колонками
	}

	// 4. Формируем итоговую строку
	var sb strings.Builder

	// Пишем заголовок
	for i, h := range headers {
		if i == len(headers)-1 {
			sb.WriteString(h) // Последний столбец без лишних пробелов в конце
		} else {
			sb.WriteString(fmt.Sprintf("%-*s", widths[i], h))
		}
	}
	sb.WriteString("\n")

	// Пишем строки с данными
	for _, row := range rows {
		for i, val := range row {
			if i == len(row)-1 {
				sb.WriteString(val) // Последний столбец без лишних пробелов в конце
			} else {
				sb.WriteString(fmt.Sprintf("%-*s", widths[i], val))
			}
		}
		sb.WriteString("\n")
	}

	return sb.String()
}

// prepareSize форматирует размер файла (2000 bytes -> 1.8KB)
func prepareSize(sizeInBytes uint64) string {
	//byte - 1 level
	//KB - 2 level
	//MB - 3 level
	//GB - 4 level

	currSize := float64(sizeInBytes)
	level := 1

	for {
		if currSize >= 1024 && level < 4 {
			currSize = currSize / 1024
			level++
		} else {
			break
		}
	}

	sizeType := "bytes"
	if level == 2 {
		sizeType = "KB"
	} else if level == 3 {
		sizeType = "MB"
	} else if level == 4 {
		sizeType = "GB"
	}

	if level == 1 {
		return fmt.Sprintf("%d %s", sizeInBytes, sizeType)
	}

	return fmt.Sprintf("%.1f %s", currSize, sizeType)
}

// prepareLines форматирует количество строк (140000 -> 140K, 41200000 -> 41.2M)
func prepareLines(lines uint64) string {
	if lines >= 1_000_000 {
		return fmt.Sprintf("%.1fM", float64(lines)/1_000_000.0)
	}
	if lines >= 1_000 {
		return fmt.Sprintf("%dK", lines/1_000)
	}
	return fmt.Sprintf("%d", lines)
}

// statusToString переводит статус из C в понятную строку
func statusToString(status cppengine.FileStatus) string {
	switch status {
	case cppengine.FileOK:
		return "OK"
	case cppengine.FileOpenError:
		return "OPEN_ERROR"
	case cppengine.FileReadError:
		return "READ_ERROR"
	case cppengine.FileInternalError:
		return "INTERNAL_ERROR"
	default:
		return "UNKNOWN"
	}
}
