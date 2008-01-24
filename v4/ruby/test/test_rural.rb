unless defined? $ZENTEST
  require 'test/unit'
  require 'rural'
end

require 'stringio'

class TestRural < Test::Unit::TestCase
  def test_version
    assert_equal Rural::VERSION.class, String
  end
end

class TestVlerq < Test::Unit::TestCase
  def test_class_cmdlist
    assert Vlerq.cmdlist.size > 70
  end

  def test_class_dispatch
    assert_respond_to Vlerq, :dispatch
  end

  class TestRow < Test::Unit::TestCase
    def setup
      @meta = View.mdesc 'A,B:I'
    end

    def test__i
      assert_equal @meta[1]._i, 1
    end

    def test__v
      assert_equal @meta[1]._v, @meta
    end

    def test_index
      assert_equal @meta[1][0], 'B'
    end
    
    def test_compare
      assert_equal(  0, @meta[0] <=> @meta[0] )
      assert_equal( -1, @meta[0] <=> @meta[1] )
      assert_equal(  1, @meta[1] <=> @meta[0] )
    end
  end

  class TestCol < Test::Unit::TestCase
    def setup
      @meta = View.mdesc 'A,B:I,C:F'
    end

    def test_each_1
      out = []
      @meta.getcol(0).each { |r| out << r }
      assert_equal out, %w[A B C]
    end

    def test_each_2
      assert_equal @meta.getcol(0).inject {|x,y| x+y }, 'ABC'
    end

    def test_index
      assert_equal @meta.getcol(0)[1], 'B'
    end
  end
end

class TestView < Test::Unit::TestCase
  def setup
    @meta = View.mdesc 'A,B:I'
    @meta1 = View.mdesc 'A,B'
    @meta2 = View.mdesc 'A,C:I'
    @meta3 = View.mdesc 'A'
    @meta4 = View.mdesc 'B:I,A'
    @view = View.data @meta, %w[a bb ccc], [1, 22, 333]
    @view1 = View.data @meta, %w[a bb a], [10, 2, 3]
  end
  
  def test_compare
    assert_equal(  0, @meta  <=> @meta  )
    assert_equal( -1, @meta  <=> @meta1 )
    assert_equal(  1, @meta1 <=> @meta  )
    assert_equal( -1, @meta  <=> @meta2 )
    assert_equal(  1, @meta2 <=> @meta  )
    assert_equal( -1, @meta1 <=> @meta2 )
    assert_equal(  1, @meta2 <=> @meta1 )
    assert_equal(  1, @meta  <=> @meta3 )
    assert_equal( -1, @meta3 <=> @meta  )
    assert_equal( -1, @meta  <=> @meta4 )
    assert_equal(  1, @meta4 <=> @meta  )
  end

  def test_structure
    assert_equal 'SSV', @meta.structure
    assert_equal 'SI', @view.structure
  end

  def test_class_method_missing_1
    assert_equal @view.length, 3
  end
  
  def test_class_method_missing_2
    assert_equal @view.size, 3
  end
  
  def test_class_method_missing_3
    assert_raise(NoMethodError) { @view.foobar }
  end

  def test_each_1
    out = []
    @view.each { |r| out << r.B }
    assert_equal out, [1, 22, 333]
  end

  def test_each_2
    assert_equal @view.collect { |r| r.B }, [1, 22, 333]
  end

  def test_dump_1
    assert_equal <<-EOF.chomp, @meta.dump
  name  type  subv
  ----  ----  ----
  A     S       #0
  B     I       #0
  ----  ----  ----
    EOF
  end

  def test_dump_2
    assert_equal <<-EOF.chomp, @view.dump
  A    B  
  ---  ---
  a      1
  bb    22
  ccc  333
  ---  ---
    EOF
  end
  
  def test_inspect
    assert_equal '#<View: 3 rows, structure: A:S,B:I>', @view.inspect
  end

  def test_project_1
    assert_equal <<-EOF.chomp, @view.project('B').meta.dump
  name  type  subv
  ----  ----  ----
  B     I       #0
  ----  ----  ----
    EOF
  end
  
  def test_project_2
    assert_equal <<-EOF.chomp, @view.project('B', 'A').meta.dump
  name  type  subv
  ----  ----  ----
  B     I       #0
  A     S       #0
  ----  ----  ----
    EOF
  end
  
  def test_project_3
    assert_equal <<-EOF.chomp, @view.project('B').dump
  B  
  ---
    1
   22
  333
  ---
    EOF
  end
  
  def test_project_4
    assert_equal <<-EOF.chomp, @view.project('B', 'A').dump
  B    A  
  ---  ---
    1  a  
   22  bb 
  333  ccc
  ---  ---
    EOF
  end

  def test_sort_1
    assert_equal <<-EOF.chomp, @view.sort.dump
  A    B  
  ---  ---
  a      1
  bb    22
  ccc  333
  ---  ---
    EOF
  end
  
  def test_sort_2
    assert_equal <<-EOF.chomp, @view1.sort.dump
  A   B 
  --  --
  a    3
  a   10
  bb   2
  --  --
    EOF
  end

  def test_emit
    assert_equal 58, @view.emit.length
  end
  
  def test_write_1
    f = 'blah'
    assert_equal 58, @view.write(f)
    assert_equal 62, f.length
    assert f[4..11] =~ /^(JL|LJ)\x1A\x00\x00\x00\x00\x3A$/
    assert_equal 0x80, f[-8]
    assert_equal 0x80, f[-16]
  end
  
  def test_write_2
    f = StringIO.new('?'*70)
    f << 'foo'
    assert_equal 58, @view.write(f)
    assert_equal 61, f.pos
    assert_equal 70, f.size
  end
  
  def test_save
    assert_equal 58, @view.reverse.save('test.dat')
    assert_equal <<-EOF.chomp, View.open('test.dat').dump
  A    B  
  ---  ---
  ccc  333
  bb    22
  a      1
  ---  ---
    EOF
    File.delete('test.dat')
  end
end
