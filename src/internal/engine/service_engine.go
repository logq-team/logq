package engine

import (
	"errors"
)

func HandleHelp() string {
	return "logq --grep [PATTERN] [PATH_TO_ARCHIVE] - processing logs by pattern\n" +
		"logq --overview [PATH_TO_ARCHIVE] [--all] - simple logs processing, counting WARN and ERROR (--all - show results for all files)"
}

func HandleGrep(pattern string, archiveName string) error {
	return errors.New("Soon...")
}
