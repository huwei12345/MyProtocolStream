#include "MyProtocolStream.h"
#include "MyProtocolStream.h"
#include <string.h>
#include <arpa/inet.h>
#include <cmath>
namespace net
{

    uint64_t htonll(uint64_t num)
    {
        uint64_t result = 0;
        char *x = (char *)&result;
        char *y = (char *)&num;
        for (int i = 0; i < 8; i++)
        {
            x[i] = y[8 - i - 1];
        }
        return result;
    }
    uint64_t ntohll(uint64_t num)
    {
        return htonll(num);
    }

    // 计算校验和
    unsigned short checksum(const unsigned short *buffer, int size)
    {
        unsigned int cksum = 0;
        while (size > 1)
        {
            cksum += *buffer++;
            size -= sizeof(unsigned short);
        }

        if (size)
        {
            cksum += *(unsigned char *)buffer;
        }
        // 将32位数转换成16
        while (cksum >> 16)
            cksum = (cksum >> 16) + (cksum & 0xffff);

        return (unsigned short)(~cksum);
    }

    // 将一个4字节的整型数值压缩成1~5个字节
    void write7BitEncoded(uint32_t value, std::string &buf)
    {
        do
        {
            unsigned char c = (unsigned char)(value & 0x7F);
            value >>= 7;
            if (value)
                c |= 0x80;

            buf.append(1, c);
        } while (value);
    }

    // 将一个8字节的整型值编码成1~10个字节
    void write7BitEncoded(uint64_t value, std::string &buf)
    {
        do
        {
            unsigned char c = (unsigned char)(value & 0x7F);
            value >>= 7;
            if (value)
                c |= 0x80;

            buf.append(1, c);
        } while (value);
    }

    // 将一个1~5个字节的字符数组值还原成4字节的整型值
    void read7BitEncoded(const char *buf, uint32_t len, uint32_t &value)
    {
        char c;
        value = 0;
        int bitCount = 0;
        int index = 0;
        do
        {
            c = buf[index];
            uint32_t x = (c & 0x7F);
            x <<= bitCount;
            value += x;
            bitCount += 7;
            ++index;
        } while (c & 0x80);
    }

    // 将一个1~10个字节的值还原成4字节的整型值
    void read7BitEncoded(const char *buf, uint32_t len, uint64_t &value)
    {
        char c;
        value = 0;
        int bitCount = 0;
        int index = 0;
        do
        {
            c = buf[index];
            uint64_t x = (c & 0x7F);
            x <<= bitCount;
            value += x;
            bitCount += 7;
            ++index;
        } while (c & 0x80);
    }

    MyProtocolStream::MyProtocolStream(std::string &str) : m_str(str), m_pos(0)
    {
        m_str.append(PACKET_LENGTH + CHECKSUM_LENGTH, 0);
        m_pos = PACKET_LENGTH + CHECKSUM_LENGTH;
    }

    bool MyProtocolStream::loadCString(const char *cstr, int len)
    {
        std::string buf;
        write7BitEncoded((uint32_t)len, buf);
        m_str.append(buf);

        m_str.append(cstr, len);
        return false;
    }

    bool MyProtocolStream::loadString(const std::string &str)
    {
        return loadCString(str.c_str(), str.length());
    }

    bool MyProtocolStream::loadFloat(float f, bool reverse)
    {
        char doublestr[128];
        if (reverse == false)
        {
            sprintf(doublestr, "%f", f);
            loadCString(doublestr, strlen(doublestr));
        }
        else
            loadCString(doublestr, 0);
        return true;
    }

    bool MyProtocolStream::loadDouble(double d, bool reverse)
    {
        char doublestr[128];
        if (reverse == false)
        {
            sprintf(doublestr, "%lf", d);
            loadCString(doublestr, strlen(doublestr));
        }
        else
            loadCString(doublestr, 0);
        return true;
    }

    bool MyProtocolStream::loadInt64(int64_t i, bool reverse)
    {
        char *p = &m_str[0];
        if (reverse == false)
        {
            int64_t ii = htonll(i);
            m_str.append((char *)&ii, sizeof(ii));
        }
        else
            m_str.append((char *)&i, sizeof(i));
        return true;
    }

    bool MyProtocolStream::loadInt32(int32_t i, bool reverse)
    {
        char *p = &m_str[0];
        if (reverse == false)
        {
            int32_t ii = htonl(i);
            m_str.append((char *)&ii, sizeof(ii));
        }
        else
            m_str.append((char *)&i, sizeof(i));
        return true;
    }

    bool MyProtocolStream::loadShort(short s, bool reverse)
    {
        char *p = &m_str[0];
        if (reverse == false)
        {
            int64_t ss = htons(s);
            m_str.append((char *)&ss, sizeof(ss));
        }
        else
            m_str.append((char *)&s, sizeof(s));
        return true;
    }

    bool MyProtocolStream::loadChar(char c, bool reverse)
    {
        m_str += c;
        return true;
    }

    bool MyProtocolStream::getCString(char *cstr, int &outlen, int maxLen)
    {
        size_t headlen;
        size_t fieldlen; // 该字符串的长度
        if (!readLengthWithoutOffset(headlen, fieldlen))
        {
            return false;
        }
        // user buffer is not enough
        if (maxLen != 0 && fieldlen > maxLen)
        {
            return false;
        }
        m_pos += headlen;
        char *cur = &m_str[m_pos];
        if (m_pos + fieldlen > m_str.length())
        {
            outlen = 0;
            return false;
        }
        cstr = cur;
        outlen = fieldlen;
        m_pos += headlen;
        return true;
    }

    bool MyProtocolStream::getString(std::string &str, int maxLen)
    {
        size_t headlen;
        size_t fieldlen;
        if (!readLengthWithoutOffset(headlen, fieldlen))
        {
            return false;
        }
        // user buffer is not enough
        if (maxLen != 0 && fieldlen > maxLen)
        {
            return false;
        }
        char *cur = &m_str[m_pos];
        cur += headlen;
        if (m_pos + fieldlen > m_str.length())
        {
            return false;
        }
        str.assign(cur, fieldlen);
        m_pos += fieldlen;
        return true;
    }

    bool MyProtocolStream::getFloat(float &f)
    {
        size_t headlen;
        size_t fieldlen;
        if (!readLengthWithoutOffset(headlen, fieldlen))
        {
            return false;
        }
        // user buffer is not enough
        char *cur = &m_str[m_pos];
        cur += headlen;
        if (m_pos + fieldlen > m_str.length())
        {
            return false;
        }
        char tmp[128];
        memcpy(tmp, cur, fieldlen);
        tmp[fieldlen] = '\0';
        f = atof(tmp);
        m_pos += fieldlen;
        return true;
    }

    bool MyProtocolStream::getDouble(double &d)
    {
        size_t headlen;
        size_t fieldlen;
        if (!readLengthWithoutOffset(headlen, fieldlen))
        {
            return false;
        }
        // user buffer is not enough
        char *cur = &m_str[m_pos];
        cur += headlen;
        if (m_pos + fieldlen > m_str.length())
        {
            return false;
        }
        char tmp[128];
        memcpy(tmp, cur, fieldlen);
        tmp[fieldlen] = '\0';
        d = atof(tmp);
        m_pos += fieldlen;
        return true;
    }

    // 读取一个数，通过7bit压缩算法
    //@handlen 该数的长度
    //@outlen 该数值
    bool MyProtocolStream::readLengthWithoutOffset(size_t &headlen, size_t &outlen)
    {
        headlen = 0;
        const char *temp = &m_str[m_pos];
        char buf[5];
        memcpy(buf, temp, sizeof(buf));
        for (size_t i = 0; i < sizeof(buf); i++)
        {
            headlen++;
            if ((buf[i] & 0x80) == 0x00)
                break;
        }
        if (m_pos + sizeof(int64_t) > m_str.length())
            return false;

        unsigned int value;
        read7BitEncoded(buf, headlen, value);
        outlen = value;
        return true;
    }

    bool MyProtocolStream::readLength(size_t &outlen)
    {
        size_t headlen;
        if (!readLengthWithoutOffset(headlen, outlen))
        {
            return false;
        }
        m_pos += headlen;
        return true;
    }

    bool MyProtocolStream::getInt64(int64_t &i)
    {
        if (m_pos + sizeof(int64_t) > m_str.length())
            return false;
        char *p = &m_str[m_pos];
        memcpy(&i, p, sizeof(int64_t));
        i = ntohll(i);
        m_pos += sizeof(int64_t);
        return true;
    }

    bool MyProtocolStream::getInt32(int32_t &i)
    {
        if (m_pos + sizeof(int32_t) > m_str.length())
            return false;
        char *p = &m_str[m_pos];
        memcpy(&i, p, sizeof(int32_t));
        i = ntohl(i);
        m_pos += sizeof(int32_t);
        return true;
    }

    bool MyProtocolStream::getShort(short &s)
    {
        if (m_pos + sizeof(short) > m_str.length())
            return false;
        char *p = &m_str[m_pos];
        memcpy(&s, p, sizeof(short));
        s = ntohs(s);
        m_pos += sizeof(short);
        return true;
    }

    bool MyProtocolStream::getChar(char &c)
    {
        if (m_pos + sizeof(char) > m_str.length())
            return false;
        c = m_str[m_pos];
        m_pos += sizeof(char);
        return true;
    }

    size_t MyProtocolStream::getAll(char *szBuffer, size_t iLen) const
    {
        size_t iRealLen = std::min(iLen, m_str.length());
        memcpy(szBuffer, m_str.c_str(), iRealLen);
        return iRealLen;
    }

    const char *MyProtocolStream::getData() const
    {
        return &m_str[0];
    }

    void MyProtocolStream::flush()
    {
        char *ptr = &m_str[0];
        unsigned int ulen = htonl(m_str.length());
        memcpy(ptr, &ulen, sizeof(ulen));
    }

    void MyProtocolStream::clear()
    {
        m_str.clear();
        m_str.append(PACKET_LENGTH + CHECKSUM_LENGTH, 0);
        m_pos = PACKET_LENGTH + CHECKSUM_LENGTH;
    }

    int MyProtocolStream::size() const
    {
        return m_str.length();
    }
    // 是否该流没有
    bool MyProtocolStream::empty() const
    {
        return m_str.length() <= PACKET_LENGTH + CHECKSUM_LENGTH;
    }
    // 是否解析完
    bool MyProtocolStream::isEnd() const
    {
        return m_pos >= m_str.length();
    }
    // 返回到某个之前的解析位置
    void MyProtocolStream::setPos(int pos)
    {
        m_pos = pos + PACKET_LENGTH + CHECKSUM_LENGTH;
    }
    // 截断
    void MyProtocolStream::trucate(int pos)
    {
        m_str.erase(pos + PACKET_LENGTH + CHECKSUM_LENGTH);
        if (m_pos > m_str.length())
        {
            m_pos = 0;
        }
    }

    MyProtocolStream& MyProtocolStream::operator<<(const std::string &str) {
        loadString(str);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(float f) {
        loadFloat(f);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(double d) {
        loadDouble(d);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(int64_t i) {
        loadInt64(i);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(int32_t i) {
        loadInt32(i);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(short s) {
        loadShort(s);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator<<(char c) {
        loadChar(c);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(std::string &str) {
        getString(str);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(float& f) {
        getFloat(f);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(double& d) {
        getDouble(d);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(int64_t& i) {
        getInt64(i);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(int32_t& i) {
        getInt32(i);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(short& s) {
        getShort(s);
        return *this;
    }
    MyProtocolStream& MyProtocolStream::operator>>(char& c) {
        getChar(c);
        return *this;
    }


} // end of namespace net
