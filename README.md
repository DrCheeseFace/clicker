# clicker
mouse oriented text editor

# TODO
- [x] gap buffers for text
- [x] write delete char
- [x] auto expanding buffer space
	- [x] buffers use utf8
	- [ ] offset lookup table for utf8 if slow?
- [x] draw cursor
- [ ] write to buffer basics
- [ ] mouse shit
	- [ ] mouse state struct/ mouse combo implementations
	- [ ] option wheel
	- [ ] change buffers
- [ ] tags cscope xref etags?
    - [ ] go to definition
    - [ ] find references
- [ ] suggested changes impl
    - [ ] autocomplete buffer dropdown
    - [ ] suggested changes from current buffer
    - [ ] suggested changes from project
- [ ] mutex on readwrite to buffers for async
- [x] some kind of memory curruption for window resize and quick events? no valgrind error
