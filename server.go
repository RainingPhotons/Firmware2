package main

import (
	"fmt"
	"math/rand"
	"net"
	"time"
)

func wheel(WheelPos int, dim int) []byte{
	r := 0
	g := 0
	b := 0
	if 85 > WheelPos {
		r = 0
		g = WheelPos * 3/dim
		b = (255 - WheelPos * 3)/dim
	} else if 170 > WheelPos {
		r = WheelPos * 3/dim
		g = (255 - WheelPos * 3)/dim
		b = 0
	} else {
		r = (255 - WheelPos * 3)/dim
		g = 0
		b = WheelPos * 3/dim
	}

	color := make([]byte, 3)
	color[0] = byte(r)
	color[1] = byte(g)
	color[2] = byte(b)
	return color
}

func main() {
	rand.Seed(time.Now().UnixNano())

	LightAddr, err := net.ResolveUDPAddr("udp", "192.168.1.209:5000")
	if err != nil {
		fmt.Println("Error: ", err)
		return
	}

	conn, err := net.DialUDP("udp", nil, LightAddr)
	if err != nil {
		fmt.Println("Error: ", err)
		return
	}

	defer conn.Close()

	buf := make([]byte, 120 * 3)
	dim := rand.Intn(2) + 4
	for j := 0; j < 256; j++ {

		for i := 0; i < 120; i++ {
			ledColor := wheel(((i * 256 / 120) + j) % 256, dim)
			buf[(i * 3) + 0] = ledColor[0]
			buf[(i * 3) + 1] = ledColor[1]
			buf[(i * 3) + 2] = ledColor[2]
		}

		conn.Write(buf)
		time.Sleep(10 * time.Millisecond)
	}
}
