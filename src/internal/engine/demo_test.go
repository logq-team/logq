package engine

//func TestDemo(t *testing.T) {
//	tests := []struct {
//		name     string
//		text     string
//		number   int64
//		bytes    uint64
//		newlines uint64
//		doubled  int64
//	}{
//		{name: "ascii", text: "hello", number: 21, bytes: 5, doubled: 42},
//		{name: "empty", text: "", number: 0, bytes: 0, doubled: 0},
//		{name: "utf8", text: "Привет", number: -7, bytes: 12, doubled: -14},
//		{name: "newlines", text: "a\nb\n", number: 3, bytes: 4, newlines: 2, doubled: 6},
//		{name: "embedded NUL", text: "a\x00b\n", number: 3, bytes: 4, newlines: 1, doubled: 6},
//		{name: "largest valid", text: "x", number: math.MaxInt64 / 2, bytes: 1, doubled: math.MaxInt64 - 1},
//		{name: "smallest valid", text: "x", number: math.MinInt64 / 2, bytes: 1, doubled: math.MinInt64},
//	}
//	for _, tt := range tests {
//		t.Run(tt.name, func(t *testing.T) {
//			got, err := Demo(tt.text, tt.number)
//			if err != nil {
//				t.Fatal(err)
//			}
//			want := DemoResult{Bytes: tt.bytes, Newlines: tt.newlines, Doubled: tt.doubled, CPPStandard: 202302}
//			if got != want {
//				t.Fatalf("Demo() = %+v, want %+v", got, want)
//			}
//		})
//	}
//}
//
//func TestDemoOverflow(t *testing.T) {
//	for _, number := range []int64{math.MaxInt64/2 + 1, math.MinInt64/2 - 1, math.MaxInt64, math.MinInt64} {
//		result, err := Demo("text", number)
//		if !errors.Is(err, ErrOverflow) {
//			t.Fatalf("Demo(_, %d) error = %v, want ErrOverflow", number, err)
//		}
//		if result != (DemoResult{}) {
//			t.Fatalf("Demo(_, %d) returned a result on overflow: %+v", number, result)
//		}
//	}
//}
