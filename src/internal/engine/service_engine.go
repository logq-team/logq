package engine

import (
	"errors"
	"logq/src/internal/archive"
)

func HandleOverview(archiveName string) error {
	_, err := archive.UnpackArchive(archiveName)

	if err != nil {
		return err
	}

	println("Overview op")
	return nil
}

func HandleHelp() string {
	return "grep [PATTERN] [PATH_TO_ARCHIVE] - разбор логов по паттерну\n" +
		"logq overview [PATH_TO_ARCHIVE] - простой разбор логов, подсчет WARN и ERROR"
}

func HandleGrep(pattern string, archiveName string) error {
	return errors.New("Soon...")
}
