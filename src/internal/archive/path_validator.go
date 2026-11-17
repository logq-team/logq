package archive

import "strings"

const (
	ParentStart = "../"
	ParentEnd   = "/.."
)

// ru:
// валидация строки
// чего не должно быть в строке:
// .. - родительского каталога
// eng:
// path validation
// the string must not contain:
// .. - parent catalog
func ValidatePath(path string) bool {
	pathLen := len(path)
	parentEndLen := len(ParentEnd)
	parentStartLen := len(ParentStart)
	noParentDir := strings.Contains(path, "/../") ||
		path == ".." ||
		(pathLen >= parentEndLen && path[pathLen-parentEndLen:] == ParentEnd) ||
		(pathLen >= parentStartLen && path[:parentStartLen] == ParentStart)

	return !noParentDir
}
