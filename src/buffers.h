#ifndef INTERNALS_H
#define INTERNALS_H

#define MAX_BUFFERS 16

// TODO: how to handle large files
// TODO: fatass struct here?
typedef char *TextBuffer;

// TODO: should this be externed
extern TextBuffer const Buffers[MAX_BUFFERS];

#endif
