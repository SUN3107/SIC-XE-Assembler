#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <bits/stdc++.h>

using namespace std;

string get_String(char c)
{
  string s(1, c);
  return s;
}

string int_ToString_Hex(int x, int fill = 5)
{
  stringstream s;
  s << setfill('0') << setw(fill) << hex << x;
  string temp = s.str();
  temp = temp.substr(temp.length() - fill, fill);
  transform(temp.begin(), temp.end(), temp.begin(), ::toupper);
  return temp;
}

string expand_String(string data, int length, char fillChar, bool back = false)
{
  if (back)
  {
    if (length <= data.length())
    {
      return data.substr(0, length);
    }
    else
    {
      for (int i = length - data.length(); i > 0; i--)
      {
        data += fillChar;
      }
    }
  }
  else
  {
    if (length <= data.length())
    {
      return data.substr(data.length() - length, length);
    }
    else
    {
      for (int i = length - data.length(); i > 0; i--)
      {
        data = fillChar + data;
      }
    }
  }
  return data;
}
int string_Hex_To_Int(string x)
{
  return stoul(x, nullptr, 16);
}

string string_To_Hex_String(const string &input)
{
  static const char *const lut = "0123456789ABCDEF";
  size_t len = input.length();

  string output;
  output.reserve(2 * len);
  for (size_t i = 0; i < len; ++i)
  {
    const unsigned char c = input[i];
    output.push_back(lut[c >> 4]);
    output.push_back(lut[c & 15]);
  }
  return output;
}

bool check_White_Space(char x)
{
  if (x == ' ' || x == '\t')
  {
    return true;
  }
  return false;
}

bool check_Comment_Line(string line)
{
  if (line[0] == '.')
  {
    return true;
  }
  return false;
}

bool if_all_num(string x)
{
  bool all_num = true;
  int i = 0;
  while (all_num && (i < x.length()))
  {
    all_num &= isdigit(x[i++]);
  }
  return all_num;
}

void read_First_Non_White_Space(string line, int &index, bool &status, string &data, bool readTillEnd = false)
{
  data = "";
  status = true;
  if (readTillEnd)
  {
    data = line.substr(index, line.length() - index);
    if (data == "")
    {
      status = false;
    }
    return;
  }
  while (index < line.length() && !check_White_Space(line[index]))
  {
    data += line[index];
    index++;
  }

  if (data == "")
  {
    status = false;
  }

  while (index < line.length() && check_White_Space(line[index]))
  {
    index++;
  }
}

void read_Byte_Operand(string line, int &index, bool &status, string &data)
{
  data = "";
  status = true;
  if (line[index] == 'C')
  {
    data += line[index++];
    char identifier = line[index++];
    data += identifier;
    while (index < line.length() && line[index] != identifier)
    {
      data += line[index];
      index++;
    }
    data += identifier;
    index++;
  }
  else
  {
    while (index < line.length() && !check_White_Space(line[index]))
    {
      data += line[index];
      index++;
    }
  }

  if (data == "")
  {
    status = false;
  }

  while (index < line.length() && check_White_Space(line[index]))
  {
    index++;
  }
}

void write_To_File(ofstream &file, string data, bool newline = true)
{
  if (newline)
  {
    file << data << endl;
  }
  else
  {
    file << data;
  }
}

string get_Real_Opcode(string opcode)
{
  if (opcode[0] == '+' || opcode[0] == '@')
  {
    return opcode.substr(1, opcode.length() - 1);
  }
  return opcode;
}

char get_Flag_Format(string data)
{
  if (data[0] == '#' || data[0] == '+' || data[0] == '@' || data[0] == '=')
  {
    return data[0];
  }
  return ' ';
}

class Evaluate_String
{
public:
  int getResult();
  Evaluate_String(string data);

private:
  string storedData;
  int index;
  char peek();
  char get();
  int term();
  int factor();
  int number();
};

Evaluate_String::Evaluate_String(string data)
{
  storedData = data;
  index = 0;
}

int Evaluate_String::getResult()
{
  int result = term();
  while (peek() == '+' || peek() == '-')
  {
    if (get() == '+')
    {
      result += term();
    }
    else
    {
      result -= term();
    }
  }
  return result;
}

int Evaluate_String::term()
{
  int result = factor();
  while (peek() == '*' || peek() == '/')
  {
    if (get() == '*')
    {
      result *= factor();
    }
    else
    {
      result /= factor();
    }
  }
  return result;
}

int Evaluate_String::factor()
{
  if (peek() >= '0' && peek() <= '9')
  {
    return number();
  }
  else if (peek() == '(')
  {
    get();
    int result = getResult();
    get();
    return result;
  }
  else if (peek() == '-')
  {
    get();
    return -factor();
  }
  return 0;
}

int Evaluate_String::number()
{
  int result = get() - '0';
  while (peek() >= '0' && peek() <= '9')
  {
    result = 10 * result + get() - '0';
  }
  return result;
}

char Evaluate_String::get()
{
  return storedData[index++];
}

char Evaluate_String::peek()
{
  return storedData[index];
}
