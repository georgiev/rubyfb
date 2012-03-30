module Rubyfb
  class Options
    @@fb15_compat = false
    def self.fb15_compat
      @@fb15_compat
    end
    def self.fb15_compat=(value)
      @@fb15_compat = value
    end
    @@charset_name_map = {
      'BIG_5' =>           'Big5',
      'GB_2312' =>         'GB2312',
      'GBK' =>            'GBK',
      'KOI8U' =>           'KOI8-U',
      'ISO8859_1' =>         'ISO-8859-1',
      'ISO8859_2' =>         'ISO-8859-2',
      'ISO8859_3' =>         'ISO-8859-3',
      'ISO8859_4' =>         'ISO-8859-4',
      'ISO8859_5' =>         'ISO-8859-5',
      'ISO8859_6' =>         'ISO-8859-6',
      'ISO8859_7' =>         'ISO-8859-7',
      'ISO8859_8' =>         'ISO-8859-8',
      'ISO8859_9' =>         'ISO-8859-9',
      'ISO8859_10' =>        'ISO-8859-10',
      'ISO8859_11' =>        'ISO-8859-11',
      'ISO8859_12' =>        'ISO-8859-12',
      'ISO8859_13' =>        'ISO-8859-13',
      'SJIS' =>           'Windows-31J',
      'UTF8' =>           'UTF-8',
      'UNICODE_FSS' =>    'UTF-8',
      'DOS866' =>         'IBM866',
      'WIN874' =>         'Windows-874',
      'WIN1250' =>        'Windows-1250',
      'WIN1251' =>        'Windows-1251',
      'WIN1252' =>        'Windows-1252',
      'WIN1253' =>        'Windows-1253',
      'WIN1254' =>        'Windows-1254',
      'WIN1255' =>        'Windows-1255',
      'WIN1256' =>        'Windows-1256',
      'WIN1257' =>        'Windows-1257',
      'WIN1258' =>        'Windows-1258'
    }.tap do |h|
      h.default = 'ASCII-8BIT'
    end
    
    def self.charset_name_map
      @@charset_name_map
    end
  end
end

