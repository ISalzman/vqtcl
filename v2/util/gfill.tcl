#!/usr/bin/env tclkit
# gfill -=- Create a MK datafile with the gadfly test data

file delete gtest.db
mk::file open db gtest.db

mk::view layout db.frequents {drinker bar perweek:I}
mk::view layout db.likes {drinker beer perday:I}
mk::view layout db.serves {bar beer quantity:I}

foreach {x y z} {
  adam lolas 1
  woody cheers 5
  sam cheers 5
  norm cheers 3
  wilt joes 2
  norm joes 1
  lola lolas 6
  norm lolas 2
  woody lolas 1
  pierre frankies 0
} {
  mk::row append db.frequents drinker $x bar $y perweek $z
}

foreach {x y z} {
  adam bud 2
  wilt rollingrock 1
  sam bud 2
  norm rollingrock 3
  norm bud 2
  nan sierranevada 1
  woody pabst 2
  lola mickies 5
} {
  mk::row append db.likes drinker $x beer $y perday $z
}

foreach {x y z} {
  cheers bud 500
  cheers samaddams 255
  joes bud 217
  joes samaddams 13
  joes mickies 2222
  lolas mickies 1515
  lolas pabst 333
  winkos rollingrock 432
  frankies snafu 5
} {
  mk::row append db.serves bar $x beer $y quantity $z
}

mk::file commit db
