#include <Arduino.h>
//khai báo thư viện để kết nối wifi cho esp32 webserver
#include <WiFi.h>
//khai báo chân sử dụng led 
const int led1 = 26;
const int led2 = 27;
 
const char* ssid = "TEN_WIFI";
const char* password = "MAT_KHAU";
//Tạo một web server tại cổng 80 - cổng mặc định cho esp32 webserver
WiFiServer webServer(80);
 
String led1Status = "OFF";
String led2Status = "OFF";
 
String header;
 
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
 
void setup() {
  Serial.begin(115200);
 
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
 
  Serial.print("Connecting to wifi");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
   
  //khởi tạo webserver
  webServer.begin();
}
 
void loop() {
  WiFiClient webClient = webServer.available(); // nếu có client kết nối với esp32 webserver
 
  if(webClient)
  {
    //khoi tao gia tri ban dau cho time
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New web Client");
    //biến lưu giá trị response
    String currentLine = "";
    //nếu có client connect và không quá thời gian time out
    while(webClient.connected() && currentTime - previousTime <= timeoutTime)
    {
      //đọc giá trị timer tại thời điểm hiện tại
      currentTime = millis();
      //nếu client còn kết nối
      if(webClient.available())
      {
        //đọc giá trị truyền từ client theo từng byte kiểu char
        char c = webClient.read();
        Serial.write(c);
        header += c; // lưu giá trị vào Header
        if(c == '\n') //Nếu đọc được kí tự xuống dòng (hết chuỗi truyền tới)
        {
          if (currentLine.length() == 0) 
          {
            //esp32 webserver response
            // HTTP headers luôn luôn bắt đầu với code HTTP (ví d HTTP/1.1 200 OK)
            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-type:text/html"); // sau đó là kiểu nội dụng mà client gửi tới, ví dụ này là html
            webClient.println("Connection: close"); // kiểu kết nối ở đây là close. Nghĩa là không giữ kết nối sau khi nhận bản tin
            webClient.println();
 
            // nếu trong file header có giá trị
            if (header.indexOf("GET /led1/on") >= 0) 
            {
              Serial.println("Led1 on");
              led1Status = "on";
              digitalWrite(led1, HIGH);
            } 
            else if (header.indexOf("GET /led1/off") >= 0) 
            {
              Serial.println("Led1 off");
              led1Status = "off";
              digitalWrite(led1, LOW);
            } 
            else if (header.indexOf("GET /led2/on") >= 0) 
            {
              Serial.println("Led2 on");
              led2Status = "on";
              digitalWrite(led2, HIGH);
            } 
            else if (header.indexOf("GET /led2/off") >= 0) 
            {
              Serial.println("Led2 off");
              led2Status = "off";
              digitalWrite(led2, LOW);
            }
            // Response trang HTML của esp32 webserver
            webClient.println("<!DOCTYPE html><html>");
            webClient.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            //thêm font-awesome 
            webClient.println("<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">");
            // code CSS cho web
            //css cho toan bo trang 
              webClient.println("<img src="data:image/gif;base64,R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7" data-wp-preserve="%3Cstyle%3Ehtml%20%7B%20font-family%3A%20Helvetica%3B%20display%3A%20inline-block%3B%20margin%3A%200px%20auto%3B%20text-align%3A%20center%3B%7D%22)%3B%0A%20%20%20%20%20%20%20%20%20%20%20%20%20%20%2F%2Fcss%20cho%20nut%20nhan%0A%20%20%20%20%20%20%20%20%20%20%20%20%20%20webClient.println(%22.button%20%7B%20background-color%3A%20%234CAF50%3B%20border%3A%20none%3B%20color%3A%20white%3B%20padding%3A%2016px%2040px%3B%22)%3B%0A%20%20%20%20%20%20%20%20%20%20%20%20%20%20webClient.println(%22text-decoration%3A%20none%3B%20font-size%3A%2030px%3B%20margin%3A%202px%3B%20cursor%3A%20pointer%3B%7D%22)%3B%0A%20%20%20%20%20%20%20%20%20%20%20%20%20%20webClient.println(%22.button2%20%7Bbackground-color%3A%20%235e0101%3B%7D%3C%2Fstyle%3E" data-mce-resize="false" data-mce-placeholder="1" class="mce-object" width="20" height="20" alt="&lt;style&gt;" title="&lt;style&gt;" /></head>");
             
            // Web Page Heading H1 with CSS
            webClient.println("<body><h1 style=\"color:Tomato;\">ESP32 Station Web Server</h1>");
 
            // Web Page Heading H2
            webClient.println("<h2 style=\"color:#077a39;\"><a href=\"https://khuenguyencreator.com\">khuenguyencreator.com</a></h2>");
            webClient.println("<i class=\"fa fa-home\" aria-hidden=\"true\"></i>");
 
            // Display current state, and ON/OFF buttons for Led1
            webClient.println("<p>Led1 - State " + led1Status + "</p>");
            // If the Led1Status is off, it displays the ON button       
            if (led1Status=="off") 
            {
              //khởi tạo một nút nhấn có đường dẫn đích là /led1/on
              webClient.println("<p><a href=\"/led1/on\"><button class=\"button\">ON</button></a></p>");
            } 
            else
            {
              //khởi tạo một nút nhấn có đường dẫn đích là /led1/off
              webClient.println("<p><a href=\"/led1/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
                
            // Display current state, and ON/OFF buttons for Led2
            webClient.println("<p>Led2 - State " + led2Status + "</p>");
            // If the led2 is off, it displays the ON button       
            if (led2Status=="off") 
            {
              //khởi tạo một nút nhấn có đường dẫn đích là /led2/on
              webClient.println("<p><a href=\"/led2/on\"><button class=\"button\">ON</button></a></p>");
            } 
            else
            {
              //khởi tạo một nút nhấn có đường dẫn đích là /led2/on
              webClient.println("<p><a href=\"/led2/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            webClient.println("</body></html>");
             
            // The HTTP response ends with another blank line
            webClient.println();
            // Break out of the while loop
            break;
          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')   //nếu giá trị gửi tới khác xuống duòng
        {
          currentLine += c;     //lưu giá trị vào biến
        }
      }
    }
    // Xoá header để sử dụng cho lần tới
    header = "";
    // ngắt kết nối với client trên esp32 webserver
    webClient.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
 
  }
}