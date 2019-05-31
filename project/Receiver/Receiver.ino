// Libraries
#include <LiquidCrystal.h>

// Pin Assignments
#define   rx_data             3
#define   rx_clock            4
#define   rx_request_resend   5

#define   lcd_d4              7  
#define   lcd_d5              8 
#define   lcd_d6              9
#define   lcd_d7              10 
#define   lcd_rs              11
#define   lcd_en              12

// total_message and Byte Variables
String total_message = "";
String message = "";

byte rx_byte = 00000000;
int bit_pos = 0;
int char_pos = 0;
char byte_char;
char terminate_char = '*';

// Byte Parity Error Check Variables 0=Even / 1=Odd
bool sent_byte_parity = false;
bool calc_byte_parity = false;

// CheckSum Error Check Variables
int checksum = 0;
int final_byte = 0;

// LDC Object
LiquidCrystal LCD_R(lcd_rs,lcd_en,lcd_d4,lcd_d5,lcd_d6,lcd_d7);

// =================================================================================
void setup() 
{
    pinMode(rx_data             , INPUT);
    pinMode(rx_clock            , INPUT);
    pinMode(rx_request_resend   , OUTPUT);

    // LCD Initialize 
    LCD_R.begin(16,2);
    LCD_CLEAR_ALL();
    
}
// =================================================================================
void loop()
{
    // --------------------------------------
    //Wait For Data Signal to Start (tx_clock)
    // --------------------------------------
    while(digitalRead(rx_clock) == LOW){}
        
    // --------------------------------------
    // Receive Bit When Clock is High
    // --------------------------------------
    
    if( bit_pos < 7 && bit_pos >= 0 )
    {
        // Read Bit
        bool rx_bit = digitalRead(rx_data);
        
        // Print Bit
        LCD_R.setCursor(bit_pos,1);
        LCD_R.print(rx_bit ? "1":"0");

        // Update Byte
        rx_byte = rx_byte | ( rx_bit << (6 - bit_pos)  );
        bit_pos++;   
    }
    else if(bit_pos == 7)
    {
        // Reset Parities
        sent_byte_parity = false;
        calc_byte_parity = false;

        // Receive Parity Checksum Bit
        sent_byte_parity = digitalRead(rx_data);

        // Print Parity Checksum Bit
        LCD_R.setCursor(7,1);
        LCD_R.print(sent_byte_parity ? "1":"0");
        bit_pos++;
        
        // Calculate Received Byte Parity
        for(  int bit_i = 0 ; bit_i < 8 ; bit_i++ )
        {
            bool rx_bit = rx_byte & (0x80 >> bit_i);
            if(rx_bit == 1){  calc_byte_parity = !calc_byte_parity;}
        }

        // Compare Parities
        if(sent_byte_parity != calc_byte_parity)
        {
            // If Parities Dont Match, Send "ReSend Byte Signal"
            digitalWrite(rx_request_resend,HIGH);
        }
    }

    // --------------------------------------
    //Wait For Data Signal To End
    // --------------------------------------
    while(digitalRead(rx_clock) == HIGH){}

    // --------------------------------------
    // Off Time (tx_clock is LOW)
    // --------------------------------------
    
    // If Parity is Correct
    if( bit_pos == 8 && digitalRead(rx_request_resend) == LOW )
    {
        // Calculate Checksum if Final Byte
        if(final_byte == 1)
        {
            checksum = checksum % 127;
            int sent_checksum = rx_byte;
            if(checksum != sent_checksum)
            {
                // Delete Message
                LCD_R.setCursor(char_pos - message.length(),0);
                LCD_R.print("                ");
                
                char_pos = total_message.length();
                
                // If CheckSums Dont Match, Send "ReSend Message Signal"
                digitalWrite(rx_request_resend,HIGH);
                delay(5);
                digitalWrite(rx_request_resend,LOW);
            }
            else
            {
                total_message += message; 
            }
            
            // Reset 
            final_byte = 0;
            checksum = 0;
            message = "";
        }
        
        // Set Final Byte Flag if Zero Byte
        else if( rx_byte == 0  )
        {
            final_byte = 1;
        }

        // Print Character if Printable
        else
        {
            byte_char = rx_byte;
            if(byte_char <= 126 && byte_char >= 32)
            {
                LCD_R.setCursor(char_pos,0);
                LCD_R.print(byte_char);
                char_pos++;
                message += byte_char;
                checksum += byte_char;
            }
        }
        
        // Reset Bit Position and LCD Byte
        rx_byte = 00000000;
        bit_pos = 0;
        LCD_CLEAR_BYTE(); 
    }
    
    // If Parity is Incorrect
    else if(bit_pos == 8 && digitalRead(rx_request_resend) == HIGH)
    {
        // Reset Bit Position and LCD Byte
        rx_byte = 00000000;
        bit_pos = 0;
        LCD_CLEAR_BYTE(); 
        
        // Disable Resend Byte Request
        digitalWrite(rx_request_resend,LOW);
    }
    
    // Erase Screen if Terminating Character is Sent
    if( total_message.indexOf('*') != -1 )
    {
        LCD_CLEAR_TOP();
        char_pos = 0;
        total_message = "";
        message = "";
    }
      
}

// =================================================================================

// --------------------------------------
// LCD Clearing Functions
// --------------------------------------
void LCD_CLEAR_ALL()
{
    LCD_R.setCursor(0,0);
    LCD_R.print("                ");
    LCD_R.setCursor(0,1);
    LCD_R.print("                ");
}
void LCD_CLEAR_TOP()
{
    LCD_R.setCursor(0,0);
    LCD_R.print("                ");
}
void LCD_CLEAR_BOTTOM()
{
    LCD_R.setCursor(0,1);
    LCD_R.print("                ");
}
void LCD_CLEAR_BYTE()
{
    LCD_R.setCursor(0,1);
    LCD_R.print("        ");
}
