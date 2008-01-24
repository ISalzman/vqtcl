require 'vlerq'

module Rural
  VERSION = '0.1.1'
end

module Vlerq
  class Col
    # so ZenTest sees it...
  end
  
  class Row
    undef_method :type

    def method_missing(m, *args)
      self[m.to_s]
    end
    
    def <=>(r)
      _v.rowcmp _i, r._v, r._i
    end
  end
end

class View
  undef_method :clone, :sort
  
  Map = Hash.new {|h,k| fail NoMethodError, "unknown view operator '#{k}'" }
  Vlerq.cmdlist.each_with_index {|s,i| Map[s.to_sym] = i }

  Map[:[]] = Map[:atrow]
  Map[:<=>] = Map[:compare]
  Map[:length] = Map[:size]

  def method_missing(m, *args)
    Vlerq.dispatch(Map[m], self, *args)
  end

  def self.method_missing(m, *args)
    Vlerq.dispatch(Map[m], *args)
  end
  
  def dump 
    out = Array.new(2 + size, '')
    
    width.times do |i|
      values = [names[i], '-']
      values += case
                  types[i]
                when 'B'
                  map {|r| "#{r[i].size}b" }
                when 'V'
                  map {|r| "##{r[i].size}" }
                else
                  map {|r| r[i].to_s }
                end

      widest = values.max {|x,y| x.size <=> y.size }
      width = [widest.size, 50].min
      align = '-' unless /[BILFDV]/ =~ types[i]
      fmt = "  %#{align}#{width}.#{width}s"

      values[0] = values[0].ljust(width)
      values[1] *= width
      
      values.each_with_index {|s,i| out[i] += fmt % s }
    end
    
    (out << out[1]) * "\n"
  end

  def inspect
    "#<View: #{size} rows, structure: #{structdesc}>"
  end
  
  def emit
    f = ''
    write(f)
    f
  end
  
  def save(filename)
    open(filename, 'wb') {|f| write(f) }
  end
  
  def project(*cols)
    colmap(cols).unique
  end
end
