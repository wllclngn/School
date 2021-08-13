package main

import (
	"bufio"
	"fmt"
	"io/ioutil"
	"os"
	"strconv"
)

/* FILE READER */
func fetchFile(x string, y bool) {

	// https://stackoverflow.com/questions/43843477/scanln-in-golang-doesnt-accept-whitespace

	data, err := ioutil.ReadFile(x)
	if err != nil {
		fmt.Println("File input ERROR:", err)
		return
	}
	// CONVERT DATA TO []string, TRIM "\n" CHARACTERS
	fmt.Print(data)
	var dataStr []string
	setup := ""
	for i := 0; i < len(data); i++ {
		// 32
		// COMMA AND SPACE
		if data[i] == 32 && data[i+1] == 32 {
			/* REMOVE REDUNDANT SPACES */
		spaceJammer:
			for j := i; j < len(data); j++ {
				if data[j] == 32 && data[j+1] == 32 {
					i++
					continue
				} else {
					dataStr = append(dataStr, setup)
					setup = ""
					break spaceJammer
				}
			}
		} else if data[i] == 44 && data[i+1] == 32 {
			/* REMOVE COMMA FOLLOWED BY SPACE */
			dataStr = append(dataStr, setup)
			setup = ""
			i++
			continue
		} else if data[i] == 13 && data[i+1] == 10 {
			/* REMOVE NEWLINE AND CARRIAGE RETURN CHARACTERS */
			dataStr = append(dataStr, setup)
			dataStr = append(dataStr, "@")
			setup = ""
			i++
			continue
		} else if data[i] == 32 || data[i] == 44 {
			/* REMOVE SPACE AND COMMA STRAGGLERS */
			dataStr = append(dataStr, setup)
			setup = ""
		} else if i == len(data)-1 {
			/* WRITE END OF FILE DATA TO dataStr */
			setup = setup + string(data[i])
			dataStr = append(dataStr, setup)
			setup = ""
		} else {
			/* TEMPORARY STORAGE FOR data TO dataStr */
			setup = setup + string(data[i])
		}
	}
	stallDesignate(dataStr, y)

	return
}

func stallDesignate(x []string, y bool) {

	stallSl := []string{}
	stallStr := "STALL"
	for i := 0; i < len(x); i++ {
	secondLoop:
		for j := i; j < len(x); j++ {
			if x[j] == "@" && i == 0 {
				i = j
				break secondLoop
			}
			if x[j] == "@" && ((j - i) > 3) {
				//thirdLoop:
				for k := j + 1; k < len(x); k++ {
					switch {
					case x[k+1] == x[k-2] || x[k+1] == x[k-3] || x[k+1] == x[k-4]:
						stallSl = append(stallSl, stallStr)
						stallSl = append(stallSl, stallStr)
						//stallSl = append(stallSl, x[k-1])
						stallSl = append(stallSl, x[k])
						i = k
						break secondLoop
					case x[k+2] == x[k-2] || x[k+2] == x[k-3] || x[k+2] == x[k-4]:
						stallSl = append(stallSl, stallStr)
						stallSl = append(stallSl, stallStr)
						//stallSl = append(stallSl, x[k-1])
						stallSl = append(stallSl, x[k])
						i = k
						break secondLoop
					case x[k+3] == x[k-2] || x[k+3] == x[k-3] || x[k+3] == x[k-4]:
						stallSl = append(stallSl, stallStr)
						stallSl = append(stallSl, stallStr)
						//stallSl = append(stallSl, x[k-1])
						stallSl = append(stallSl, x[k])
						i = k
						break secondLoop
					}
				}
			}
			if x[j] == "@" && ((j - i) >= 2) {
				//thirdLoop:
				for l := j + 1; l < len(x); l++ {
					switch {
					case x[l+1] == x[l-2] || x[l+1] == x[l-3] || x[l+1] == x[l-4] || x[l+1] == x[l-5] || x[l+1] == x[l-6]:
						stallSl = append(stallSl, stallStr)
						stallSl = append(stallSl, stallStr)
						//stallSl = append(stallSl, x[l-1])
						stallSl = append(stallSl, x[l])
						i = l
						break secondLoop
					case x[l+2] == x[l-2] || x[l+2] == x[l-3] || x[l+2] == x[l-4] || x[l+2] == x[l-5] || x[l+2] == x[l-6]:
						stallSl = append(stallSl, stallStr)
						stallSl = append(stallSl, stallStr)
						//stallSl = append(stallSl, x[l-1])
						stallSl = append(stallSl, x[l])
						i = l
						break secondLoop
					}
				}
			}
			if x[j] != "@" {
				stallSl = append(stallSl, x[j])
			}
		}
	}
	stallSl = append(stallSl[:len(stallSl)-1])
	stallSl2 := []string{}
	for l := 0; l < len(stallSl); l++ {
		if stallSl[l] != "@" {
			stallSl2 = append(stallSl2, stallSl[l])
		}
	}

	riscSim(stallSl, y)

	return

}

func translateOperation(x []string, y []string) ([]string, []string) {

	switch {
	case y[0] == "REGC":
		y[0] = "0"
	case y[0] == "REGB":
		y[0] = "1"
	case y[0] == "REGA":
		y[0] = "2"
	case y[0] == "MEM5":
		y[0] = "3"
	case y[0] == "MEM4":
		y[0] = "4"
	case y[0] == "MEM3":
		y[0] = "5"
	case y[0] == "MEM2":
		y[0] = "6"
	case y[0] == "MEM1":
		y[0] = "7"
	}

	for i := 1; i < len(y); i++ {
		switch {
		case y[i] == "REGC":
			okay, _ := strconv.ParseInt(x[0], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "REGB":
			okay, _ := strconv.ParseInt(x[1], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "REGA":
			okay, _ := strconv.ParseInt(x[2], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "MEM5":
			okay, _ := strconv.ParseInt(x[3], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "MEM4":
			okay, _ := strconv.ParseInt(x[4], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "MEM3":
			okay, _ := strconv.ParseInt(x[5], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "MEM2":
			okay, _ := strconv.ParseInt(x[6], 2, 64)
			y[i] = strconv.Itoa(int(okay))
		case y[i] == "MEM1":
			/* WORKS */
			okay, _ := strconv.ParseInt(x[7], 2, 64)
			y[i] = strconv.Itoa(int(okay))
			//y[i] = strconv.FormatInt(newInt64, 2)
		}

	}
	return x, y
}

// ***** BINARY TO HEX CONVERSION ***** //

func binToHex(b []string) []string {

	binInt := 0
	power := 0
	binSlice := []string{}

	for i := len(b) - 1; i >= 0; i-- {
		for j := len(b[i]) - 1; j >= 0; j-- {
			if int(b[i][j]) == 49 {
				binInt += 1 * (1 << power)
			}
			power++
		}
		binStr := strconv.Itoa(binInt)
		binSlice = append([]string{binStr}, binSlice...)
		binInt = 0
		power = 0
	}

	value2 := []int{}
	value3 := ""
	value4 := []string{}

	fmt.Println(binSlice)
	for k := 0; k < len(binSlice); k++ {
		fmt.Println(value2)
		fmt.Println(value2)
		fmt.Println(value2)
		fmt.Println(value2)
		fmt.Println(value2)
		fmt.Println(value2)
		value, _ := strconv.Atoi(binSlice[k])
		if value > 9 {
			for l := 0; l < 9223372036854775807; l++ {
				if value >= 16 {
					y := value % 16
					value2 = append(value2, y)
					value >>= 4
				} else {
					value2 = append(value2, value)
					break
				}
			}
			for m := len(value2) - 1; m >= 0; m-- {
				switch {
				case value2[m] == 10:
					value3 = value3 + "A"
				case value2[m] == 11:
					value3 = value3 + "B"
				case value2[m] == 12:
					value3 = value3 + "C"
				case value2[m] == 13:
					value3 = value3 + "D"
				case value2[m] == 14:
					value3 = value3 + "E"
				case value2[m] == 15:
					value3 = value3 + "F"
				default:
					z := strconv.Itoa(value2[m])
					value3 = value3 + z
				}
			}

		} else {
			z := strconv.Itoa(value)
			value3 = value3 + z
		}
		value4 = append(value4, value3)
		value3 = ""
		value2 = []int{}
	}

	value5 := systemCheck(value4)

	return value5
}

func systemCheck(x []string) []string {
	len0 := 0
	temp := ""
	for i := 0; i < len(x); i++ {
		switch {
		case len(x[i]) < 8:
			len0 = len(x[i])
			temp = ""
			for i := len0; i < 8; i++ {
				temp = temp + "0"
			}
			temp = temp + x[i]
			hexTemp := "0x"
			hexTemp = hexTemp + temp
			x[i] = hexTemp
		}
	}

	return x
}

/* RISC SIMULATOR */
func riscSim(x []string, y bool) {

	system := []string{"0000000000000000000000000000000000000000",
		"0000000000000000000000000000000000000000",
		"0000000000000000000000000000000000000000",
		"000000000000",
		"000000000000",
		"000000000000",
		"000000000010",
		"000000000001",
		"000000",
		"0000",
		"0000"}

	tempJunk := []string{"", "", ""}
	loadVar := []string{}

	// string >> INSTRUCTIONS, MEM, REGISTER byte 0,1
	//  ***** DECIMAL TO BINARY ***** //
	for i := 0; i < len(x); i++ {
		fmt.Println(x[i:])
		switch {
		case x[i] == "ZERO":
			system[8] = "000000"
		case x[i] == "IMM":
			system[8] = "000001"
		case x[i] == "ADD":
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 + moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[results1] = logJunk2
			system[8] = "000010"
			i += 2
		case x[i] == "SUB":
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 - moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[results1] = logJunk2
			system[8] = "000011"
			i += 2
		case x[i] == "MULT":
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 * moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[results1] = logJunk2
			system[8] = "000100"
			i += 3
		case x[i] == "MULTHS":
			/* ENTER 32-bit, RETURN 64-BIT */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 * moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "000101"
			system[results1] = logJunk2
			i += 3
		case x[i] == "MULTHSU":
			/* ENTER 32-bit, RETURN 64-BIT */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 * moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "000110"
			system[results1] = logJunk2
			i += 3
		case x[i] == "DIV":
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 / moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "000111"
			system[results1] = logJunk2
			i += 3
		case x[i] == "DIVS":
			/* NEGATIVE TO POSTIIVE RANGE */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 / moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "001000"
			system[results1] = logJunk2
			i += 3
		case x[i] == "DIVUNS":
			/* ZERO TO MAX POSITIVE */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			logJunk := moreJunk1 / moreJunk2
			logJunk2 := strconv.FormatInt(int64(logJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "001001"
			system[results1] = logJunk2
			i += 3
		case x[i] == "STOREBYTE":
			/* REG TO MEM */
			tempJunk[0], tempJunk[1] = x[i+1], x[i+2]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			logJunk := strconv.FormatInt(int64(moreJunk1), 2)
			loadVar := tempJunk[0]
			loadVarInt, _ := strconv.Atoi(loadVar)
			system[loadVarInt] = logJunk
			system[8] = "010000"
			i += 2

		/**************************************
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		HIT AND MISS
		***************************************/
		case x[i] == "STOREHALF":
			/* REG TO MEM */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "010001"
			system[results1] = subJunk2
			i += 3
		case x[i] == "STOREWORD":
			/* REG TO MEM */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "010010"
			system[results1] = subJunk2
			i += 3
		case x[i] == "LOADBYTE":
			/* DONE */
			/* MEM TO REG */
			tempJunk[0], tempJunk[1] = x[i+1], x[i+2]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			logJunk := strconv.FormatInt(int64(moreJunk1), 2)
			loadVar := tempJunk[0]
			loadVarInt, _ := strconv.Atoi(loadVar)
			system[loadVarInt] = logJunk
			system[8] = "010011"
			i += 2
		case x[i] == "LOADBYTEUN":
			/* DONE */
			/* MEM TO REG */
			tempJunk[0], tempJunk[1] = x[i+1], x[i+2]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			logJunk := strconv.FormatInt(int64(moreJunk1), 2)
			loadVar := tempJunk[0]
			loadVarInt, _ := strconv.Atoi(loadVar)
			system[loadVarInt] = logJunk
			system[8] = "010100"
			i += 2
		case x[i] == "LOADHALF":
			/* MEM TO REG */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "010101"
			system[results1] = subJunk2
			i += 3
		case x[i] == "LOADHALFUN":
			/* MEM TO REG */
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "010110"
			system[results1] = subJunk2
			i += 3
		case x[i] == "LOADWORD":
			/* MEM TO REG */
			tempJunk[0], tempJunk[1] = x[i+1], x[i+2]
			translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			logJunk := strconv.FormatInt(int64(moreJunk1), 2)
			loadVar := tempJunk[0]
			loadVarInt, _ := strconv.Atoi(loadVar)
			system[loadVarInt] = logJunk
			system[8] = "010111"
			i += 2
		case x[i] == "SETLESSTHAN":
			/* The	immediate	value	(a	sign-extended	12-bit	value,	i.e.,	-2,048	..	+2,047)	is
			compared	to	the	contents	of	Reg1	using	signed	comparison.	If	the	value	in
			Reg1	is	less	than	the	immediate	value,	the	value	1	is	stored	in	RegD.
			Otherwise,	the	value	0	is	stored	in	RegD.*/
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "011000"
			system[results1] = subJunk2
			i += 3
		case x[i] == "SETLESSTHANUN":
			tempJunk[0], tempJunk[1], tempJunk[2] = x[i+1], x[i+2], x[i+3]
			//translateOperation(system, tempJunk)
			moreJunk1, _ := strconv.Atoi(tempJunk[1])
			moreJunk2, _ := strconv.Atoi(tempJunk[2])
			subJunk := moreJunk1 - moreJunk2
			subJunk2 := strconv.FormatInt(int64(subJunk), 2)
			results1, _ := strconv.Atoi(tempJunk[0])
			system[8] = "011001"
			system[results1] = subJunk2
			i += 3
		case x[i] == "SHIFTLEFTLOG":
			tempJunk[0] = x[i+1]
			_, logJunk := translateOperation(system, tempJunk)
			results1, _ := strconv.Atoi(logJunk[0])
			tempJunk[0] = x[i+2]
			_, logJunk2 := translateOperation(system, tempJunk)
			logJunk3, _ := strconv.Atoi(logJunk2[0])
			logJunk4 := system[logJunk3]
			logJunk4 = logJunk4[1:] + "0"
			system[8] = "011100"
			system[results1] = logJunk4
			i += 2
		case x[i] == "SHIFTRIGHTLOG":
			tempJunk[0] = x[i+1]
			_, logJunk := translateOperation(system, tempJunk)
			results1, _ := strconv.Atoi(logJunk[0])
			tempJunk[0] = x[i+2]
			_, logJunk2 := translateOperation(system, tempJunk)
			logJunk3, _ := strconv.Atoi(logJunk2[0])
			logJunk4 := system[logJunk3]
			logJunk4 = "0" + logJunk4[:len(logJunk4)-2]
			system[8] = "011100"
			system[results1] = logJunk4
			i += 2
		// ***** WORKS ***** //
		// ***** WORKS ***** //
		// ***** WORKS ***** //
		// ***** WORKS ***** //
		// ***** WORKS ***** //
		// ***** WORKS ***** //
		case x[i] == "SHIFTRIGHARTH":
			tempJunk[0] = x[i+1]
			_, logJunk := translateOperation(system, tempJunk)
			results1, _ := strconv.Atoi(logJunk[0])
			tempJunk[0] = x[i+2]
			_, logJunk2 := translateOperation(system, tempJunk)
			logJunk3, _ := strconv.Atoi(logJunk2[0])
			logJunk4 := system[logJunk3]
			logJunk4 = "1" + logJunk4[:len(logJunk4)-1]
			system[8] = "011100"
			system[results1] = logJunk4
			i += 2
		case x[i] == "SHIFTLEFTARTH":
			tempJunk[0] = x[i+1]
			_, logJunk := translateOperation(system, tempJunk)
			results1, _ := strconv.Atoi(logJunk[0])
			tempJunk[0] = x[i+2]
			_, logJunk2 := translateOperation(system, tempJunk)
			logJunk3, _ := strconv.Atoi(logJunk2[0])
			logJunk4 := system[logJunk3]
			logJunk4 = logJunk4 + "0"
			system[8] = "011101"
			system[results1] = logJunk4
			i += 2
		case x[i] == "AIN":
			/* ANYWEHRE TO REG A */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			tempAIN, _ := strconv.Atoi(loadVar[0])
			system[2] = system[tempAIN]
			loadVar = []string{}
			system[8] = "011110"
			i++
		case x[i] == "LOADA":
			/* MEM TO REG A */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[2] = system[loadVarInt]
			system[8] = "011111"
			i++
		case x[i] == "AOUT":
			/* REG A TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[2]
			system[8] = "100000"
			i++
		case x[i] == "BIN":
			/* ANYWEHRE TO REG B */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[1] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "100001"
			i++
		case x[i] == "LOADB":
			/* MEM TO REG B */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[1] = system[loadVarInt]
			system[8] = "100010"
			i++
		case x[i] == "BOUT":
			/* REG B TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[1]
			system[8] = "100011"
			i++
		case x[i] == "CIN":
			/* ANYWEHRE TO REG C */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[1] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "100100"
			i++
		case x[i] == "LOADC":
			/* MEM TO REG C */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[1] = system[loadVarInt]
			system[8] = "100101"
			i++
		case x[i] == "COUT":
			/* REG C TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[0]
			system[8] = "100110"
			i++
		case x[i] == "MEM1IN":
			/* ANYWEHRE TO REG MEM1 */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[7] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "100111"
			i++
		case x[i] == "MEM1OUT":
			/* MEM1 TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[7]
			system[8] = "100110"
			i++
		case x[i] == "MEM2IN":
			/* ANYWEHRE TO REG MEM2 */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[6] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "101001"
			i++
		case x[i] == "MEM2OUT":
			/* MEM2 TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[6]
			system[8] = "101010"
			i++
		case x[i] == "MEM3IN":
			/* ANYWEHRE TO REG MEM3 */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[5] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "101011"
			i++
		case x[i] == "MEM3OUT":
			/* MEM3 TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[5]
			system[8] = "101100"
			i++
		case x[i] == "MEM4IN":
			/* ANYWEHRE TO REG MEM4 */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[4] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "101101"
			i++
		case x[i] == "MEM4OUT":
			/* MEM4 TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[4]
			system[8] = "101110"
			i++
		case x[i] == "MEM5IN":
			/* ANYWEHRE TO REG MEM5 */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[4] = system[loadVarInt]
			loadVar = []string{}
			system[8] = "101111"
			i++
		case x[i] == "MEM5OUT":
			/* MEM5 TO ANYWHERE */
			tempJunk[0] = x[i+1]
			_, loadVar = translateOperation(system, tempJunk)
			loadVarInt, _ := strconv.Atoi(loadVar[0])
			system[loadVarInt] = system[3]
			system[8] = "110000"
			i++
		case x[i] == "JUMP":
			system[8] = "110001"
		case x[i] == "STALL":
			system[8] = "0"
			system[8] = "111110"
		case x[i] == "RESET":
			system = []string{"0000000000000000000000000000000000000000",
				"0000000000000000000000000000000000000000",
				"0000000000000000000000000000000000000000",
				"000000000000",
				"000000000000",
				"000000000000",
				"000000000000",
				"000000000000",
				"111111",
				"0000",
				"0000"}
		}

		system2 := binToHex(system)
		if y == true {
			fmt.Println(system2)
			fmt.Println("PRESS ENTER WHENEVER YOU WISH TO CONTINUE.")
			stepScan := bufio.NewScanner(os.Stdin)
		keepOnSteppin:
			for stepScan.Scan() {
				stepper := stepScan.Text()
				fmt.Printf("INPUT: %q\n", stepper)
				break keepOnSteppin
			}
		}

	}
	/*
		// 32-bit
		// ****** FLAGS ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		0000
		1111

		FLAGS:
		Carry, Sign/Unsigned, Zero, Negative

		Zero                    0001
		Not Zero                0010
		Carry                   0011
		Not Carry               0100
		SignedOverflow          0101
		Not SignedOverflow      0110
		Negative                0111

		// ****** OPCODES ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		1111
		0000

		ZERO        0000
		LOAD        0001
		STALL       0010
		SKIP        0011
		JUMP        0100
		HALT        0101
		----        0110
		----        0111
		----        1000
		----        1001
		----        1010
		----        1011
		----        1100
		----        1101
		END         1111

		// ***** INSTRUCTIONS ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		111111
		0000
		0000

		ZERO          000000
		IMM           000001
		ADD           000010
		SUB           000011
		MULT          000100
		MULTHSS       000101
		MULTHSU       000110
		DIV           000111
		DIVS          001000
		DIVUNS        001001
		AIN           001010
		LOADA         001011
		AOUT          001100
		BIN           001101
		LOADB         001110
		BOUT          001111
		CIN           010000
		LOADC         010001
		COUT          010010
		MEM1IN        010011
		LDMEM1        010100
		MEM2IN        010101
		LDMEM2        010110
		MEM3IN        010111
		LDMEM3        011000
		MEM4IN        011001
		LDMEM4        011010
		MEM5IN        011011
		LDMEM5        011100
		AND           011101
		ANDIM         011111
		OR            100000
		ORIM          100001
		XOR           100010
		XORIM         100011
		STOREBYTE     100100
		STOREHALF     100101
		STOREWORD     100111
		LOADBYTE      101001
		LOADBYTEUN    101010
		LOADHALF      101011
		LOADHALFUN    101100
		LOADWORD      101101
		SETLESSTHAN   101111
		SETLESSTHANUN 110000
		SHIFTLEFTLOG  110001
		SHIFTRIGHTLOG 110010
		SHIFTRIGHARTH 110011
		JUMP          111001
		RESET         111111

		// ***** MEM 1 ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		111111111111
		000000
		0000
		0000

		// ***** MEM 2 ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		111111111111
		000000000000
		000000
		0000
		0000

		// ***** MEM 3 ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		111111111111
		000000000000
		000000000000
		000000
		0000
		0000

		// ***** MEM 4 ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		111111111111
		000000000000
		000000000000
		000000000000
		000000
		0000
		0000

		// ***** MEM 5 ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		111111111111
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		0000
		0000

		// ***** REG A ***** //
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		1111111111111111111111111111111111111111
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		0000
		0000

		// ***** REG B ***** //
		0000000000000000000000000000000000000000
		1111111111111111111111111111111111111111
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		0000
		0000

		// ***** REG C ***** //
		1111111111111111111111111111111111111111
		0000000000000000000000000000000000000000
		0000000000000000000000000000000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000000000
		000000
		0000
		0000
	*/
}

/* MAIN FUNCITON */
func main() {

	fmt.Print("Will Clingan\nHomework #2\nCIS 655, Dr. Mo\n6 August 2021\n\n")
	fmt.Print("Welcome to the command-line RISC-V Simulator.\n\n")
	fmt.Print("TUTORIAL: List accepted instructions for input programs, as well a few examples.\n")
	fmt.Print("RUN: Load in a new program by giving the RISC-V Simulator your file's path (with no quotes).\n")
	fmt.Print("WALK: Load in Step thru RUN instructions\n")
	fmt.Print("When ready, enter the branch you wish to take:\n")

	fetcher := ""
	scanner := bufio.NewScanner(os.Stdin)
	for scanner.Scan() {
		prompt := scanner.Text()
		fmt.Println()
		if prompt == "INSTRUCTIONS" {
			fmt.Println("********** INSTRUCTIONS **********")
			fmt.Print("********** INSTRUCTIONS **********\n\n")
			fmt.Println("Enter your next branch:")

		} else if prompt == "RUN" {
			fmt.Println("********** FETCH **********")
			fmt.Println("Enter the file's path (with no quotes):")

		fetchLoop:
			for scanner.Scan() {
				fetchPrompt := scanner.Text()
				fmt.Printf("INPUT: %q\n", fetchPrompt)
				var stepBool bool
				fmt.Println("YES or NO: Would you like to STEP thru this file?")
			stepperLoop:
				for scanner.Scan() {
					stepper := scanner.Text()
					if stepper == "YES" {
						stepBool = true
					} else {
						stepBool = false
					}
					break stepperLoop
				}
				if stepBool == true {
					fetchFile(fetchPrompt, true)
				} else {
					fetchFile(fetchPrompt, false)
				}
				fmt.Println("FETCH of input file succesful.")
				fmt.Println("********** FETCH **********")
				fmt.Print("\nEnter your next branch:\n")
				break fetchLoop
			}

		} else if prompt == "RESET" {

		} else if prompt == "STEP" {
			if fetcher == "" {
				fmt.Println("")
			}

		} else if prompt == "END" {

		}
	}
	if err := scanner.Err(); err != nil {
		fmt.Println("Error encountered:", err)
	}

}
