Test assignment for TekMonks
======

For TekMonks: Please don't skip the text below.

The code implemented is hardly a real life example is how the requested thing has to be done. From what was understood from HR it's desirable to avoid usage of any library implementing every piece of used stack manually. Onfortuantelly it's not possible since implementing TLS would take much more time than expected, so OpenSSL was linked. And it's not possible to avoid TLS since there is not `http` version of the page available.

The second moment to notice it's epoll. Yes all this asynchronous logic complicates the code but the intention was to demonstrate my knowledge, not to make something simple and worthless. And after all I like epoll, I used it in my other projects. But in the next ones I'd use some wrapper like libev or something. Well it depends on a project.

Tested on Ubuntu 19.10

Dependencies:

 * GTest
 * OpenSSL

Most likely there are a lot of issues in the code.  I didn't check it with Valgrind, I didn't test all the possible edge cases. Again it's matter of time which I don't have.
