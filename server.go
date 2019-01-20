package main

import (
	"fmt"
	"math/rand"
	"net"
	"strconv"
	"time"
)

func wheel(WheelPos int, dim int) []byte {
	r := 0
	g := 0
	b := 0
	if 85 > WheelPos {
		r = 0
		g = WheelPos * 3 / dim
		b = (255 - WheelPos*3) / dim
	} else if 170 > WheelPos {
		r = WheelPos * 3 / dim
		g = (255 - WheelPos*3) / dim
		b = 0
	} else {
		r = (255 - WheelPos*3) / dim
		g = 0
		b = WheelPos * 3 / dim
	}

	color := make([]byte, 3)
	color[0] = byte(r)
	color[1] = byte(g)
	color[2] = byte(b)
	return color
}

func rainbowCycle(conn net.Conn, wait int, cycles int, dim int) {
	buf := make([]byte, 120*3)

	for cycle := 0; cycle < cycles; cycle++ {
		dir := rand.Intn(2)
		k := 255

		for j := 0; j < 256; j++ {
			k = k - 1
			for i := 0; i < 120; i++ {
				if k < 0 {
					k = 255
				}
				offset := j
				if dir == 0 {
					offset = k
				}
				ledColor := wheel(((i*256/120)+offset)%256, dim)
				buf[(i*3)+0] = ledColor[0]
				buf[(i*3)+1] = ledColor[1]
				buf[(i*3)+2] = ledColor[2]
			}

			conn.Write(buf)
			time.Sleep(time.Duration(wait) * time.Millisecond)
		}
	}
}

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

type strand struct {
	host int
	conn net.Conn
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

		dim := rand.Intn(2) + 4
		wait := rand.Intn(20) + 10
		max_cycles := 8
		cycles := rand.Intn(max_cycles) + 1
		rainbowCycle(strands[i].conn, wait, cycles, dim)
	}
}
