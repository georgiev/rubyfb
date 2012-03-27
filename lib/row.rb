module Rubyfb
  class Row
    attr_reader :number
    
    def initialize(metadata, data, row_number)
      @number = row_number
      @metadata = metadata
      @data = data
    end
    
    def column_count
      @metadata.size
    end
    
    def column_name(idx)
      @metadata[get_index(idx)].name
    end
    
    def column_alias(idx)
      @metadata[get_index(idx)].alias
    end
    
    def column_scale(idx)
      @metadata[get_index(idx)].scale
    end
    
    def get_base_type(idx)
      @metadata[get_index(idx)].type
    end
    
    def each
      @metadata.each_with_index do |c, i|
        yield c.key, @data[i]
      end if block_given?
    end
    
    def each_key
      @metadata.each do |c|
        yield c.key
      end if block_given?
    end

    def each_value
      @data.each do |v|
        yield v
      end if block_given?
    end
    
    def [](idx)
      @data[get_index(idx)]
    end
    
    def fetch(index, default=nil)
      soft_value(index) || default || (
        if block_given?
          yield index
        else
          raise IndexError.new("Column identifier #{index} not found in row.")
        end
      )
    end
    
    def has_key?(key)
      !!@metadata.find{ |col| col.key == key }
    end

    def has_column?(name)
      !!@metadata.find{ |col| col.name == name }
    end
    
    def has_alias?(column_alias)
      !!@metadata.find{ |col| col.alias == column_alias }
    end
    
    def has_value?(value)
      !!@data.find{ |v| v == value }
    end

    def keys
      @metadata.collect{ |col| col.key }
    end

    def names
      @metadata.collect{ |col| col.name }
    end

    def aliases
      @metadata.collect{ |col| col.alias }
    end

    def values
      @data
    end

    def select
      block_given? || (raise StandardError.new("No block specified in call to Row#select."))
      
      [].tap do |a|
        @metadata.each_with_index do |c, i|
          a << [c.key, @data[i]] if yield c.key, @data[i]
        end
      end 
    end
    
    def to_a
      select{ true }
    end
    
    def to_hash
      {}.tap do |map|
        @metadata.each_with_index do |c, i|
          map[c.key] = @data[i]
        end
      end
    end
    
    def values_at(*columns)
      columns.collect{ |key| soft_value(key) }
    end

    alias_method :each_pair, :each
    alias_method :include?, :has_key?
    alias_method :key?, :has_key?
    alias_method :member?, :has_key?
    alias_method :value?, :has_value?
    alias_method :length, :column_count
    alias_method :size, :column_count
  private
    def soft_value(key)
      if index = find_index(key)
        @data[index]
      end
    end
    
    def find_index(key)
      if key.kind_of?(String)
        @metadata.find_index{ |c| c.key == key }
      else
        key < 0 ? size + key : key
      end
    end
    
    def get_index(key)
      find_index(key) || (raise IndexError.new("Column identifier #{key} not found in row."))
    end
  end
end

