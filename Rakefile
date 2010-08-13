require 'echoe'
e = Echoe.new('rubyfb', '0.5.2') do |p|
  p.description    = "Firebird SQL access library"
  p.url            = "http://rubyforge.org/projects/rubyfb"
  p.author         = "George Georgiev"
  p.email          = "georgiev@heatbs.com"
  p.rdoc_pattern   = ["{examples,ext,lib}/*.rb", "CHANGELOG", "README", "LICENSE"]
  p.need_tar_gz    = !PLATFORM.include?("win32")

  if ARGV.include? "binpkg"
    p.platform=Gem::Platform::CURRENT
    p.eval = Proc.new {
      self.extensions=nil
      self.platform=Gem::Platform::CURRENT
    }
  end
end
e.clean_pattern = e.clean_pattern - e.clean_pattern.grep(/^lib/)

task :binpkg => [:compile, :repackage]
