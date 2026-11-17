package engine

import "logq/src/internal/archive"

func HandleOverview(archiveName string) error {
	_, err := archive.UnpackArchive(archiveName)

	if err != nil {
		return err
	}

	return nil
}

func HandleHelp() string {

}
