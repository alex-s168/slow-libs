= dgtxt file format
Human readable file format for directed graphs

== Design goals
- any valid input to `tsort`, not containing `#`, is always valid dgtxt.
- removing every occureance of the RegEx `#.*?\n` should produce a valid input to `tsort` (if it doesn't contain cycles of course)
- easy to generate
- human readable
- easy to parse fully

== Concepts
=== Node
Has:
- incoming edges
- outgoing edges
- name
- key-value attributes

=== Edge
Has:
- source node
- destination node
- key-value attributes

== Syntax
Note that `{x}` means none or more times.

```ebnf
ident char = anything - '\n' - '\r' - ' ' - '#';
ident = ident char, { ident char };

node attribute = '##', {spaces},
                 ident, {spaces},  (* referred node *)
                 ident, {spaces},  (* attribute key *)
                 { not new line }  (* attribute value *)
               ;

comment = '#', { note new line };

edge attribute = '#:', {spaces}, ident, {spaces}, {not new line} ;

edge = ident, {spaces},  (* source node *)
       ident, {spaces},  (* destination node *)
       { {spaces}, edge attribute, {spaces} }
     ;

element = edge
        | node attribute
        | comment
        ;


grammar = { spaces, element, spaces };
```

== Example
```
# comment line (ignored)
DownloadLinux FlashDrive
# double hashtass start node attributes:
## DownloadLinux describtion Attrbiute key/value formats are not standardized her
FindFlashDrive    FlashDrive   #: cost 200
                               #: another-attr-key  another attr value (until end of line)
DownloadDriveFlashSoftware FlashDrive
FlashDrive ShutdownWindows
ShutdownWindows  RestartPc
RestartPc InstallLinux
```

== Recommendations
- tools should prefix attribute names with the tool names, unless they think that other tools might adopt that attribute too.
- tools that modify graphs should preserve all attributes
- tools should not error on unknown attributes (or invalid attribute values), as they might be valid in a different tool
- actual comments should have a space after the `#`, for future proofing
- node attributes be listed directly after node definitions
