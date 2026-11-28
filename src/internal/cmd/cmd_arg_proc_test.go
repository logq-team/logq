package cmd

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestProcessArgs(t *testing.T) {
	cases := []struct {
		CaseTitle       string
		Args            []string
		Successful      bool
		CommandExpected Command
	}{
		{
			"Single arg - logq, invalid usage",
			[]string{"logq"},
			false,
			"",
		},
		{
			"Args for help operation, successful",
			[]string{"logq", "--help"},
			true,
			HelpCommand,
		},
		{
			"Args for grep operation, not enough args",
			[]string{"logq", "--grep"},
			false,
			"",
		},
		{
			"Args for grep operation, successful",
			[]string{"logq", "--grep", "Exception|Timeout", "archive_title"},
			true,
			GrepCommand,
		},
		{
			"Args for overview operation, not enough args",
			[]string{"logq", "--overview"},
			false,
			"",
		},
		{
			"Args for overview operation, successful",
			[]string{"logq", "--overview", "path_to_archive"},
			true,
			OverviewCommand,
		},
		{
			"Unknown flag",
			[]string{"logq", "--helloworld"},
			false,
			"",
		},
		{
			"Invalid usage",
			[]string{"logq", "helloworld"},
			false,
			"",
		},
	}

	for _, testCase := range cases {
		t.Run(testCase.CaseTitle,
			func(t *testing.T) {
				command, err := ProcessArgs(testCase.Args)

				if testCase.Successful {
					require.NoError(t, err)
				} else {
					require.Error(t, err)
				}

				require.Equal(t, testCase.CommandExpected, command)
			})
	}
}
