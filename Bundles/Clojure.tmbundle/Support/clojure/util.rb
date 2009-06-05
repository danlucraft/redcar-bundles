module Clojure::Util
  module_function
    
  # Given a body of text, a line, and a column, returns the absolute offset of the line+column in the text
  def get_absolute_offset(str, line, col)
    split = str.split("\n")
    split[0...line-1].join("\n").length + col
  end
  
  def connect_terminal(screen_name)
    script = nil
    if ENV['TM_TERMINAL'] =~ /^iterm$/i || Clojure::Util.is_running('iTerm')
      script = iterm_script(screen_name)
    else
      script = terminal_script(screen_name)
    end
    open("|osascript", "w") { |io| io << script }
  end
  
  def is_running(process)
    all = `ps -U "$USER" -o ucomm`
    all.to_a[1..-1].find { |cmd| process == cmd.strip }
  end                     
  
  def get_clj()
    if ENV['TM_CLJ'] != nil
      return ENV['TM_CLJ']
    elsif `which clj` != ""
      return `which clj`
    else
      return File.expand_path(File.dirname(__FILE__) + '/../../Vendor/clj')
    end
  end

  def make_command(screen)
    return "screen -dm -x #{e_sh screen} || screen -S #{e_sh screen} -- '#{get_clj}' -i"
  end
  

  def terminal_script(screen)
    command = make_command(screen)
    
    return <<-APPLESCRIPT
      tell application "Terminal"
        activate
        do script "#{command}"
      end tell
  APPLESCRIPT
  end

  def iterm_script(screen)
    command = make_command(screen)
    
    return <<-APPLESCRIPT
      tell application "iTerm"
        activate
        if exists the first terminal then
          set myterm to the first terminal
        else
          set myterm to (make new terminal)
        end if
        
        tell myterm
          activate current session
          launch session "Default Session"

          tell the last session
            write text "#{command}"
          end tell
        end tell
        
      end tell
  APPLESCRIPT
  end
end
