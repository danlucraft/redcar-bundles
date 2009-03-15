
require 'open3'

module Kernel
  def `(str)
    result, error = nil, nil
    File.open("tmp.sh", "w") {|fout| fout.puts str}
    Open3.popen3("bash tmp.sh") do |stdin, stdout, stderr| 
      result = stdout.read
      error = stderr.read 
    end
    puts error
    result
  end
end
