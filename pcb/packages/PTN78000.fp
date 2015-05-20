# retain backwards compatibility to older versions of PKG_DIL 
# which did not have 100,60,28 args
Element(0x00 "PTN78000 package" "" "PTN78000" 220 100 3 100 0x00)
(
	Pin(50 50 60 28 "1" 0x101)
	Pin(50 300 60 28 "2" 0x01)
	Pin(50 425 60 28 "3" 0x01)
	Pin(675 425 60 28 "4" 0x01)
	Pin(675 50 60 28 "5" 0x01)
	ElementLine(-10 -10 735 -10 10)
	ElementLine(735 -10 735 485  10)
	ElementLine(735 485 -10 485 10)
	ElementLine(-10 485 -10 -10 10)
	Mark(50 50)
)
