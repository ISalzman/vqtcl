# Simple version.  Probably should do something about getting column
# types correct

ratcl::vopdef sqlite {db args} {
    $db eval $args r {}
    return [view $r(*) vdef [$db eval $args]]
}
