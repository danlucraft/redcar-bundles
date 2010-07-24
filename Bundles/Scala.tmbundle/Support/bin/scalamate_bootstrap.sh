#!/usr/bin/env bash

source "$TM_SUPPORT_PATH/lib/bash_init.sh"

export TM_RUBY=${TM_RUBY:-ruby}
export TM_JAVA=${TM_JAVA:-java}
export TM_JAVAC=${TM_JAVAC:-javac}
export TM_SCALA=${TM_SCALA:-scala}
export TM_SCALAC=${TM_SCALAC:-scalac}

require_cmd "$TM_JAVA" "If you have installed java, then you need to either <a href=\"help:anchor='search_path'%20bookID='TextMate%20Help'\">update your <tt>PATH</tt></a> or set the <tt>TM_JAVA</tt> shell variable (e.g. in Preferences / Advanced)"

require_cmd "$TM_JAVAC" "If you have installed javac, then you need to either <a href=\"help:anchor='search_path'%20bookID='TextMate%20Help'\">update your <tt>PATH</tt></a> or set the <tt>TM_JAVAC</tt> shell variable (e.g. in Preferences / Advanced)"

require_cmd "$TM_SCALA" "If you have installed scala, then you need to either <a href=\"help:anchor='search_path'%20bookID='TextMate%20Help'\">update your <tt>PATH</tt></a> or set the <tt>TM_SCALA</tt> shell variable (e.g. in Preferences / Advanced)"

require_cmd "$TM_SCALAC" "If you have installed scalac, then you need to either <a href=\"help:anchor='search_path'%20bookID='TextMate%20Help'\">update your <tt>PATH</tt></a> or set the <tt>TM_SCALAC</tt> shell variable (e.g. in Preferences / Advanced)"

require_cmd "$TM_RUBY" "We need Ruby to proceed."

scalamate.rb