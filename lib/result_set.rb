module Rubyfb
  class ResultSet
    def create_row(metadata, data, row_number)
      Row.new(metadata, data, row_number)
    end
  end
end
