<div align="center" id="top"> 
  <img src="./.github/app.gif" alt="Stnc" />

  &#xa0;

  <!-- <a href="https://stnc.netlify.app">Demo</a> -->
</div>

<h1 align="center">Stnc</h1>

<p align="center">
  <img alt="Github top language" src="https://img.shields.io/github/languages/top/tjhv10/stnc?color=56BEB8">

  <img alt="Github language count" src="https://img.shields.io/github/languages/count/tjhv10/stnc?color=56BEB8">

  <img alt="Repository size" src="https://img.shields.io/github/repo-size/tjhv10/stnc?color=56BEB8">
</p>

<p align="center">
  <a href="#dart-about">About</a> &#xa0; | &#xa0; 
  <a href="#sparkles-features">Features</a> &#xa0; | &#xa0;
  <a href="#checkered_flag-starting">Starting</a> &#xa0; | &#xa0;
  <a href="https://github.com/tjhv10" target="_blank">Author</a>
</p>

<br>

## :dart: About ##
This project has two parts:\
***Part A:*** the first part is a chat between a chat and a client.
You can communicate between the client and the server using the terminal.
You can see the running process for this part in the **"starting"** section.\
***Part B:*** the second part is a preformance tool. you can check 8 types of communications such as: tcp , udp , uds dgram and stream, mmap and pipe.
The Client generate a 100 MB string and send it to the server using the chosen communication.
The server measure the time the transfer action took and print it to the terminal as follows: For example if you used ipv4 tcp and the transfer took 2130 ms the server will print : ipv4_tcp,2130.
## :sparkles: Features ##
✔️ Feature 1: Chat tool that you can use send and recv without the recv will block the code from running.\
which means that you can send how many messeges you want and the server will get them all.

✔️ Feature 2: preformance toll that checks the speed of diffrent communications.

## :checkered_flag: Starting ##

```bash
# Clone this project
$ git clone https://github.com/tjhv10/stnc

# Access
$ cd stnc

# Create an exe file to run the program
$ make

# Run the project part A server:
$ ./stnc -s <port>
# Example: ./stnc -s 20123

# Run the project part A client:
$ ./stnc -c <IP> <port>
# Example: ./stnc -c 10.0.2.15 20123

# Run the project part B server:
$ ./stnc -s <port> -p(for preformance check) -q (for quiet mode)
# Example: ./stnc -s 20123 -p -q

# Run the project part B client:
$ ./stnc -c <IP> <port> -p <type> <communication>
# Example: ./stnc -c 10.0.2.15 20123 -p ipv4 tcp
```



Made with :heart: by <a href="https://github.com/tjhv10" target="_blank">Achiya and Nitay</a>

&#xa0;

<a href="#top">Back to top</a>
