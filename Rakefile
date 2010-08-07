
desc 'Clean up (sanitize) the Textmate files for packaging'
task :clean_textmate do
  # rename files to be x-platform safe
  Dir["Bundles/*.tmbundle/*/**/*"].each do |fn|
    if File.file?(fn)
      if File.extname(fn) == ""
        new_basename = File.basename(fn).gsub(" ", "_").gsub(/[^\w_]/, "__").gsub(/\\./, "__")
        new_fn = File.join(File.dirname(fn), new_basename)
      else
        bits = fn.split("/").last.split(".")[0..-2].join("_")
        new_basename = bits.gsub(" ", "_").gsub(/[^\w_]/, "__").gsub(/\\./, "__") + File.extname(fn)
        new_fn = File.join(File.dirname(fn), new_basename)
      end
      next if new_fn == fn
      if File.exist?(new_fn)
        p [fn, new_fn]
        if File.read(new_fn) == File.read(fn)
          FileUtils.rm_rf(fn)
        else
          puts "can't move or remove: #{fn} (#{new_fn})"
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
