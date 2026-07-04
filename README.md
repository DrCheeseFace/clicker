# clicker
mouse oriented text editor

# TODO
- [x] gap buffers for text
- [x] write delete char

- [x] auto expanding buffer space
	- [x] buffers use utf8
	- [ ] offset lookup table for utf8 if slow?

- [ ] text editor basics
	- [x] draw cursor
	- [ ] variable text size and display character based on font size
	- [ ] cursor movement
	- [ ] write delete to buffer
	- [ ] draw text

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
- [ ]
- [ ] mutex on readwrite to buffers for async
- [x] some kind of memory curruption for window resize and quick events? no valgrind error
