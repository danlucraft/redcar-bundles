$:.unshift File.dirname(__FILE__)
require ENV['TM_SUPPORT_PATH'] + '/lib/textmate.rb'
require ENV['TM_SUPPORT_PATH'] + '/lib/ui.rb'
require ENV['TM_SUPPORT_PATH'] + '/lib/exit_codes.rb'
require 'stringio'
require 'tempfile'

module Clojure
  require 'clojure/process'
  require 'clojure/repl'
  require 'clojure/parser'
  require 'clojure/util'
  require 'clojure/core_ext'
  
  module Mate
    TEXT_WRAP_LINE_LENGTH = 40
        
    def run_current_form_or_line
      repl = get_repl
      
      raw_text = STDIN.read
      forms = Clojure::Parser.parse(raw_text)
      
      offset =  if ENV["TM_SELECTED_TEXT"] 
                  selection = ENV["TM_SELECTED_TEXT"]
                  line = ENV["TM_INPUT_START_LINE"].to_i
                  column = ENV["TM_INPUT_START_LINE_INDEX"].to_i
                  start_offset = Util.get_absolute_offset(raw_text, line, column)
                  start_offset...(start_offset + selection.length)
                else
        
                  line =    ENV['TM_LINE_NUMBER'].to_i
                  column =  ENV['TM_LINE_INDEX'].to_i
                  Util.get_absolute_offset(raw_text, line, column)
                end
      
      to_run = pick_form(forms, offset) || ENV["TM_CURRENT_LINE"]
      result = repl.evaluate(to_run)
            
      (1..(result.length/TEXT_WRAP_LINE_LENGTH)).each do |counter|
        result[(counter * TEXT_WRAP_LINE_LENGTH)..0] = "\n"
      end
      
      show_text(result)
    end
  
    def run_file
      show_html get_repl.evaluate(STDIN.read)
    end
    
    def restart_repl
      get_repl.restart
      show_html "Restarted"
    end
    
    def get_help
      text = STDIN.read
      
      show_html get_repl.evaluate("(doc #{text})")
    end
    
    def get_classpath
      text = STDIN.read
      
      classpath = get_repl.evaluate("(System/getProperty \"java.class.path\")").
                    chomp.
                    gsub(/^"(.*)"$/,'\1').
                    split(':').
                    map { |p| File.expand_path(p) }.
                    join("\n")
      
      %w{TM_BUNDLE_SUPPORT TM_BUNDLE_PATH TM_PROJECT_DIRECTORY}.each do |e|
        classpath.gsub!(/^#{ENV[e]}/,"$#{e}")
      end
      
      show_html("CLASSPATH:\n\n"+
                classpath)
    end
    
    def macroexpand_1
      text = STDIN.read
      
      get_repl.evaluate("(macroexpand-1 '#{text})")
    end
    
    def execute_clojure
      text = STDIN.read
      
      get_repl.evaluate(text)
    end
    
    def connect_terminal
      get_repl.connect_terminal
      TextMate.exit_discard
    end
  
    private
    def get_repl
      if ENV["TM_PROJECT_DIRECTORY"] != nil
        project_name = File.basename(ENV["TM_PROJECT_DIRECTORY"])
      elsif ENV["TM_FILENAME"] != nil
        project_name = File.basename(ENV["TM_FILENAME"])
      else
        project_name = "untitled-#{rand(10000)}"
      end
      
      Clojure.ensure_running(project_name)
    end
  
    def pick_form(forms, offset)
      case offset
      when Fixnum
        key = forms.keys.find{|t| t === offset}
        forms[key]
      when Range
        keys = forms.keys.select{|key| key.overlaps?(offset)}
        keys.sort!{|l,r| l.first <=> r.first}
        keys.map do |k|
          forms[k]
        end.join("\n")
      end
    end
    
    def show_html(result)
      TextMate.exit_show_html("<pre>#{result.gsub("<", "&lt;")}</pre>")
    end
    
    def show_text(result)
      print result
    end
    
    extend self
  end
  
end


if __FILE__ == $0
  p Clojure::Mate.send(:get_repl)
end