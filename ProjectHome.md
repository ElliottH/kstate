This is a library to make it simple to share state between Linux processes. It aims to be able to cope with rapidly changing state, which may or may not be inspected at the same frequency.

A simple transaction model is supported.

A shared state may not be more than one page in size.


---


_There's code here, but it's nowhere near doing anything useful yet. I'm basically using the repository as a way to colocate development between work and home. APIs are not fixed, behaviour is not fixed (it's not necessarily even there yet!), and major ideas are even likely to change. So, very much caveat emptor._

_I shall take this notice away (or at least modify it) when there is something that other people might find of use..._

Currently, [check](http://check.sourceforge.net/) is being used to write C unit tests for the project, and this is a dependency in the Makefile (i.e.,building also runs the tests). 'check' is LGPLv2, but since the binaries produced for the unit tests are _not_ part of the final product, I believe its use is OK from a licensing point of view. Regardless, it may or may not be used in future versions of the project.