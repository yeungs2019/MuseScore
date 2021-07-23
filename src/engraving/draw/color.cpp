#include "color.h"

#include <sstream>
#include <iomanip>

static bool com = QMetaType::registerComparators<mu::draw::Color>();
static bool con = QMetaType::registerConverter<mu::draw::Color, QString>(&mu::draw::Color::toQString);

namespace mu::draw {
static constexpr int fromHex(uint c);
static int hex2int(const char* s, int n);
static bool getHexRgb(const char* name, size_t len, mu::draw::Rgba* rgba);
static std::string rgb2hex(int r, int g, int b, int a = Color::m_defaultAlpha);
static std::string rgb2hex(Rgba rgba);
static constexpr bool isValidComp(int num);
static constexpr bool isRgbaValid(int r, int g, int b, int a = Color::m_defaultAlpha);
static void insertHexComponent(int num, std::stringstream& ss);

Color::Color()
{
}

Color::Color(const Color& other)
    : m_rgba(other.m_rgba), m_isValid(other.m_isValid)
{
}

Color::Color(int r, int g, int b, int a)
    : m_rgba(rgba(r, g, b, a)), m_isValid(isRgbaValid(r, g, b, a))
{
}

Color::Color(const char* color)
{
    setNamedColor(color);
}

#ifndef NO_QT_SUPPORT
Color::Color(const QColor& color)
    : m_rgba(color.rgba()), m_isValid(color.isValid())
{
}

#endif

Color& Color::operator=(const Color& other)
{
    m_rgba = other.m_rgba;
    m_isValid = other.m_isValid;
    return *this;
}

#ifndef NO_QT_SUPPORT
Color& Color::operator=(const QColor& other)
{
    *this = fromQColor(other);
    return *this;
}

#endif

bool Color::operator==(const Color& other) const
{
    return m_rgba == other.m_rgba && m_isValid == other.m_isValid;
}

bool Color::operator!=(const Color& other) const
{
    return !operator==(other);
}

bool Color::operator<(const Color& other) const
{
    return m_rgba < other.m_rgba;
}

QString Color::toQString() const
{
    return QString::fromStdString(toString());
}

std::string Color::toString() const
{
    return rgb2hex(m_rgba);
}

bool Color::isValid() const
{
    return m_isValid;
}

int Color::red() const
{
    return getRed(m_rgba);
}

int Color::green() const
{
    return getGreen(m_rgba);
}

int Color::blue() const
{
    return getBlue(m_rgba);
}

int Color::alpha() const
{
    return getAlpha(m_rgba);
}

#ifndef NO_QT_SUPPORT
Color Color::fromQColor(const QColor& color)
{
    Color c;
    c.m_rgba = color.rgba();
    c.m_isValid = color.isValid();
    return c;
}

QColor Color::toQColor() const
{
    return QColor(m_rgba);
}

#endif

void Color::setNamedColor(const std::string& color)
{
    if (color.empty()) {
        m_isValid = false;
        return;
    }

    Rgba rgba;
    if (color[0] == '#') {
        if (getHexRgb(color.data(), color.size(), &rgba)) {
            setRgba(rgba);
        } else {
            m_isValid = false;
        }

        return;
    }

    m_isValid = false;
}

void Color::setNamedColor(const char* color)
{
    setNamedColor(std::string(color));
}

void Color::setRed(int value)
{
    setRgba(value, green(), blue(), alpha());
}

void Color::setGreen(int value)
{
    setRgba(red(), value, blue(), alpha());
}

void Color::setBlue(int value)
{
    setRgba(red(), green(), value, alpha());
}

void Color::setAlpha(int value)
{
    setRgba(red(), green(), blue(), value);
}

void Color::setRgba(int r, int g, int b, int a)
{
    if (!isRgbaValid(r, g, b, a)) {
        m_isValid = false;
        return;
    }

    m_rgba = rgba(r, g, b, a);
    m_isValid = true;
}

void Color::setRgba(Rgba rgba)
{
    if (!isRgbaValid(getRed(rgba), getGreen(rgba), getBlue(rgba), getAlpha(rgba))) {
        m_isValid = false;
        return;
    }

    m_rgba = rgba;
    m_isValid = true;
}

static constexpr int fromHex(uint c)
{
    return ((c >= '0') && (c <= '9')) ? int(c - '0')
           : ((c >= 'A') && (c <= 'F')) ? int(c - 'A' + 10)
           : ((c >= 'a') && (c <= 'f')) ? int(c - 'a' + 10)
           : -1;
}

static int hex2int(const char* s, int n)
{
    if (n < 0) {
        return -1;
    }
    int result = 0;
    for (; n > 0; --n) {
        result = result * 16;
        const int h = fromHex(*s++);
        if (h < 0) {
            return -1;
        }
        result += h;
    }
    return result;
}

static bool getHexRgb(const char* name, size_t len, mu::draw::Rgba* rgba)
{
    if (name[0] != '#') {
        return false;
    }
    name++;
    --len;
    int a, r, g, b;
    a = 65535;
    if (len == 12) {
        r = hex2int(name + 0, 4);
        g = hex2int(name + 4, 4);
        b = hex2int(name + 8, 4);
    } else if (len == 9) {
        r = hex2int(name + 0, 3);
        g = hex2int(name + 3, 3);
        b = hex2int(name + 6, 3);
        if (r == -1 || g == -1 || b == -1) {
            return false;
        }
        r = (r << 4) | (r >> 8);
        g = (g << 4) | (g >> 8);
        b = (b << 4) | (b >> 8);
    } else if (len == 8) {
        a = hex2int(name + 0, 2) * 0x101;
        r = hex2int(name + 2, 2) * 0x101;
        g = hex2int(name + 4, 2) * 0x101;
        b = hex2int(name + 6, 2) * 0x101;
    } else if (len == 6) {
        r = hex2int(name + 0, 2) * 0x101;
        g = hex2int(name + 2, 2) * 0x101;
        b = hex2int(name + 4, 2) * 0x101;
    } else if (len == 3) {
        r = hex2int(name + 0, 1) * 0x1111;
        g = hex2int(name + 1, 1) * 0x1111;
        b = hex2int(name + 2, 1) * 0x1111;
    } else {
        r = g = b = -1;
    }
    if ((uint)r > 65535 || (uint)g > 65535 || (uint)b > 65535 || (uint)a > 65535) {
        *rgba = 0;
        return false;
    }
    *rgba = mu::draw::rgba(r, g, b, a);
    return true;
}

static constexpr bool isValidComp(int num)
{
    return (num >= 0) && (num < 256);
}

static constexpr bool isRgbaValid(int r, int g, int b, int a)
{
    return isValidComp(r)
           && isValidComp(g)
           && isValidComp(b)
           && isValidComp(a);
}

static void insertHexComponent(int num, std::stringstream& ss)
{
    ss << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << num;
}

static std::string rgb2hex(int r, int g, int b, int a)
{
    std::stringstream ss;

    ss << "#";
    insertHexComponent(r, ss);
    insertHexComponent(g, ss);
    insertHexComponent(b, ss);
    if (a != Color::m_defaultAlpha) {
        insertHexComponent(a, ss);
    }
    return ss.str();
}

static std::string rgb2hex(Rgba rgba)
{
    return rgb2hex(getRed(rgba), getGreen(rgba), getBlue(rgba), getAlpha(rgba));
}
}