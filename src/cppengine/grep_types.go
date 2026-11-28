package cppengine

// GrepOptions controls engine execution and collection of matching lines
type GrepOptions struct {
	// Requested number of C++ workers with a minimum of 1
	Workers uint32
	// Count all matching lines without collecting their text
	CountOnly bool
	// Global limit on returned lines across all files
	// Must be positive unless CountOnly is true where this limit is ignored
	MaxMatches uint64
}

// GrepResult owns Go copies which remain valid after releasing the native result
type GrepResult struct {
	// Complete per-file counts and statuses in the same order as input paths
	Files []GrepFileStats
	// Matching lines ordered by file index then line number
	// Empty in CountOnly mode and otherwise limited by MaxMatches
	Matches []Match
	// True only when MaxMatches caused matching lines to be omitted
	// File counts remain complete and CountOnly always returns false here
	Truncated bool
}

// GrepFileStats corresponds to the input path at the same index
type GrepFileStats struct {
	// Total physical lines including a final nonempty line without LF
	Lines uint64
	// Lines matching at least one pattern counted once per line
	MatchingLines uint64
	// File processing status with zero counts and no matches on failure
	Status FileStatus
}

// Match contains a zero based file index and a one based line number
// Text excludes the final LF and preserves other bytes including CR
type Match struct {
	// Zero based index into input paths and GrepResult.Files
	FileIndex int
	// One based physical line number in the source file
	LineNumber uint64
	// Original line without its final LF preserving CR and embedded NUL
	// This Go string remains valid after the native result is freed
	Text string
}

// GrepFunc defines the future Grep adapter signature and permits CLI test doubles
// The native implementation and cgo adapter are provided by a later task
// paths contains prepared regular file paths in caller supplied order
// patterns contains at least one case sensitive literal substring matched with OR semantics
// options controls workers counting and the global text limit
// A non-nil error describes a call failure while individual file errors use Files[i].Status
type GrepFunc func(paths []string, patterns []string, options GrepOptions) (GrepResult, error)
