
desc 'Clean up (sanitize) the Textmate files for packaging'
task :clean_textmate do
  # rename files to be x-platform safe
  Dir["Bundles/*.tmbundle/*/**/*"].each do |fn|
    if File.file?(fn)
      bits = fn.split("/").last.split(".")[0..-2].join("_")
      new_basename = bits.gsub(" ", "_").gsub(/[^\w_]/, "__").gsub(/\\./, "__") + File.extname(fn)
      new_fn = File.join(File.dirname(fn), new_basename)
      # p [fn,new_fn]
      next if new_fn == fn
      if File.exist?(new_fn)
        new_fn = File.join(File.dirname(fn), "next_" + new_basename)  
        if File.read(new_fn) == File.read(fn)
          FileUtils.rm_rf(fn)
        else
          puts "can't move or remove: #{fn}"
        end
      else
        begin
          FileUtils.mv(fn, new_fn)
        rescue => e
          puts e
        end
      end
    end
  end
end
