# Clojure.tmbundle

## Prerequisites

The Clojure bundle depends on the following external utilities to work:

* bash
* ruby
* osascript

If either of those aren't on your Textmate's PATH, these commands will fail in unknown and spectacular fashion.

Additionally, if clj is not found on your Textmate's PATH, then it will use the one included in this bundle. You can manually specify it with the TM_CLJ variable.

## Installation

Run this:
 
	$ cd ~/Library/Application\ Support/TextMate/Bundles
	$ git clone git://github.com/stephenroller/clojure-tmbundle.git Clojure.tmbundle
	$ osascript -e 'tell app "TextMate" to reload bundles'

An update script will :

* Get Clojure and Clojure-contrib from GoogleCode.
* Get library for generating off-line documentation from github.
* Build Clojure and documentation.

Run this :

	$ ./Clojure.tmbundle/Vendor/clj-update-and-build

## Future goals

I hope to incorporate either nullstyle's REPL or make my own. The vendor clojure implementation has readline support, but not completions. I might use the clj suggested on the clojure wiki instead.

If I write my own REPL, I'll probably eliminate all the ruby out altogether. I'd prefer to find a way to collaborate with nullstyle instead though.