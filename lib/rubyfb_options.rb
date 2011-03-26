module Rubyfb
  class Options
    @@fb15_compat = false
    def self.fb15_compat
      @@fb15_compat
    end
    def self.fb15_compat=(value)
      @@fb15_compat = value
    end
  end
end

