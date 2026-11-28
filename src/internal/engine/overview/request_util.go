package overview

import (
	"errors"
	"fmt"
)

type OverviewRequest struct {
	ArchiveName string
	AllOption   bool
}

const (
	MaxArgsNum = 10
)

const (
	AllFlag = "--all"
)

/*

--------

	call for checking flags
	for overview op

--------

	important rule: first 2 args from source cmd args slice
	should be thrown away

--------

*/
func PrepareRequestForOverview(args []string) (*OverviewRequest, error) {
	var req OverviewRequest
	req.ArchiveName = args[0]

	if len(args) > MaxArgsNum {
		return nil, fmt.Errorf("Too many arguments, you can add at most %d args\n", MaxArgsNum)
	}

	for _, s := range args[1:] {
		if err := processFlag(&req, s); err != nil {
			return nil, err
		}
	}

	return &req, nil
}

func processFlag(req *OverviewRequest, s string) error {
	switch s {
	case AllFlag:
		if req.AllOption {
			return errors.New("flag \"--all\" passed more than once\n")
		}
		req.AllOption = true
		return nil
	default:
		return fmt.Errorf("Unknown flag: %s\n"+
			"Type \"logq --help\" for more information\n", s)
	}
}
