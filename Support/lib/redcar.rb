
require 'open3'

module Kernel
  def `(str)
    result, error = nil, nil
    File.open("redcar_tmp.sh", "w") {|fout| fout.puts str}
    Open3.popen3("bash redcar_tmp.sh") do |stdin, stdout, stderr| 
      result = stdout.read
      error = stderr.read 
    end
    FileUtils.rm_f("redcar_tmp.sh")
    puts error
    result
  end
end
