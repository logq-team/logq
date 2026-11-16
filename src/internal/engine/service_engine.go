package engine

import "logq/src/internal/archive"

func ProcessArchive(archiveName string) error {
	_, err := archive.UnpackArchive(archiveName)

	if err != nil {
		return err
	}

	//for _, fileN := range fileNames {
	//	println(fileN)
	//}

	return nil
}
