module Rubyfb
  class Connection
    # Creates stored procedure call object
    def prepare_call(procedure_name)
      Rubyfb::ProcedureCall.new(self, procedure_name)
    end
    
    def force_encoding(fb_str, sqlsubtype)
      fb_str
    end
  private
    def init_m17n
      return unless String.method_defined?(:force_encoding)
      
      @charset_map= Hash.new(Rubyfb::Options.charset_name_map.default)
      start_transaction do |tr|
        execute("SELECT RDB$CHARACTER_SET_NAME, RDB$CHARACTER_SET_ID FROM RDB$CHARACTER_SETS", tr) do |row|
          @charset_map[row['RDB$CHARACTER_SET_ID']]=Rubyfb::Options.charset_name_map[row['RDB$CHARACTER_SET_NAME'].strip]
        end
      end
      instance_eval do 
        def force_encoding(fb_str, sqlsubtype)
          fb_str.tap do |s|
            s.force_encoding(@charset_map[sqlsubtype])
          end
        end
      end
    end
  end
end



