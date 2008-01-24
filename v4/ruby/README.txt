rural
  http://rural.rubyforge.org/
  http://www.vlerq.org/

== DESCRIPTION:

Rural adds relational algebra operators and persistence to Ruby applications.

This is a Ruby binding to Vlerq, a vector-oriented query engine using relational
algebra and set-wise operations. The data can be either in memory or on file.

== FEATURES/PROBLEMS:

* The current code is proof of concept.

== SYNOPSYS:

  require 'rural'
  meta = View.mdesc 'A,B:I'
  view = View.data meta, ['a','bb','ccc'], [1,22,333]
  puts view.dump

== REQUIREMENTS:

* Ruby 1.8+
* Test::Unit
* Rake or rubygems for install/uninstall

== INSTALL:

Using Rubygems:

* sudo gem install rural

Using Rake:

* rake test
* sudo rake install

== LICENSE:

(The MIT License)

Copyright (c) 2006 Jean-Claude Wippler

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
