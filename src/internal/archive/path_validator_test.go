package archive

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/require"
)

func Test_PathValidator(t *testing.T) {
	cases := []struct {
		testPath string
		result   bool
	}{
		{"/dimefresh/", true},
		{"/dimefresh/dimedime", true},
		{"../dimefresh/", false},
		{"/lol/../dimefresh/", false},
		{"/dimefresh/..", false},
		{"/..", false},
		{"../", false},
		{"..", false},
	}

	for _, testCase := range cases {
		t.Run(
			fmt.Sprintf("Testing path %s\n", testCase.testPath),
			func(t *testing.T) {
				res := ValidatePath(testCase.testPath)
				require.Equal(t, testCase.result, res)
			},
		)
	}
}
