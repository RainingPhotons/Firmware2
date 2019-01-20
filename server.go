package main

import (
	"fmt"
	"math/rand"
	"net"
	"strconv"
	"time"
)

func connectToStrand(hostOctet int) (net.Conn, error) {
	ipaddr := "192.168.1." + strconv.Itoa(hostOctet) + ":5000"
	LightAddr, err := net.ResolveUDPAddr("udp", ipaddr)
	if err != nil {
		fmt.Println("Error: ", err)
		return nil, err
	}

	conn, err := net.DialUDP("udp", nil, LightAddr)
	if err != nil {
		fmt.Println("Error: ", err)
		return nil, err
	}

	return conn, nil
}

const LEDCount = 120

type strand struct {
	host int
	conn net.Conn
}

type rgb struct {
	r byte
	g byte
	b byte
}

func rain(strands [3]strand, size int) {
	matrix := make([][]byte, size)
	wait := 100
	for i := range matrix {
		matrix[i] = make([]byte, LEDCount*3)
	}

	for c := 0; c < 5000; c++ {
		for i := 0; i < size; i++ {
			for j := ((LEDCount - 1) * 3); j > 0; j -= 3 {
				matrix[i][j+0] = matrix[i][j-3+0]
				matrix[i][j+1] = matrix[i][j-3+1]
				matrix[i][j+2] = matrix[i][j-3+2]
			}
			if rand.Intn(5) == 0 {
				matrix[i][1] = byte(rand.Intn(255))
			} else {
				matrix[i][1] = 0
			}
		}

		for i := 0; i < size; i++ {
			strands[i].conn.Write(matrix[i])
		}
		time.Sleep(time.Duration(wait) * time.Millisecond)
	}
}

func main() {
	rand.Seed(time.Now().UnixNano())
	var strands [3]strand
	strands[0].host = 209
	strands[1].host = 205
	strands[2].host = 218

	for i := 0; i < len(strands); i++ {
		strands[i].conn, _ = connectToStrand(strands[i].host)

		defer strands[i].conn.Close()
	}
	rain(strands, len(strands))
}
