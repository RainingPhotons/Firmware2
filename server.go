package main

import (
	"fmt"
	"github.com/faiface/beep"
	"github.com/faiface/beep/speaker"
	"github.com/faiface/beep/wav"
	"net"
	"os"
	"time"
)

func main() {
	LightAddr, err := net.ResolveUDPAddr("udp", "192.168.1.154:5000")
	if err != nil {
		fmt.Println("Error: ", err)
		return
	}
	ServerAddr, err := net.ResolveUDPAddr("udp", ":10001")
	if err != nil {
		fmt.Println("Error: ", err)
		return
	}
	ServerConn, err := net.ListenUDP("udp", ServerAddr)
	if err != nil {
		fmt.Println("Error: ", err)
		return
	}
	defer ServerConn.Close()

	buf := make([]byte, 1024)

	sr := beep.SampleRate(48000)
	speaker.Init(sr, sr.N(time.Second/10))
	file, _ := os.Open("smw_1-up.wav")
	streamer, _, _ := wav.Decode(file)

	for {
		n, addr, err := ServerConn.ReadFromUDP(buf)
		fmt.Println("Received ", string(buf[0:n]), " from ", addr)
		streamer.Seek(0)
		speaker.Play(streamer)
		if err != nil {
			fmt.Println("Error: ", err)
		}
		ServerConn.WriteToUDP(buf, LightAddr)
	}
}
