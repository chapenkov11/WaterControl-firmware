#include <stdint.h>

#define MAKE_PORT(portName, ddrName, pinName, className, ID)  \
    class className                                           \
    {                                                         \
    public:                                                   \
        typedef uint8_t DataT;                                \
                                                              \
    private:                                                  \
        static volatile DataT &data()                         \
        {                                                     \
            return portName;                                  \
        }                                                     \
        static volatile DataT &dir()                          \
        {                                                     \
            return ddrName;                                   \
        }                                                     \
        static volatile DataT &pin()                          \
        {                                                     \
            return pinName;                                   \
        }                                                     \
                                                              \
    public:                                                   \
        static void Write(DataT value)                        \
        {                                                     \
            data() = value;                                   \
        }                                                     \
        static void ClearAndSet(DataT clearMask, DataT value) \
        {                                                     \
            data() = (data() & ~clearMask) | value;           \
        }                                                     \
        static DataT Read()                                   \
        {                                                     \
            return data();                                    \
        }                                                     \
        static void DirWrite(DataT value)                     \
        {                                                     \
            dir() = value;                                    \
        }                                                     \
        static DataT DirRead()                                \
        {                                                     \
            return dir();                                     \
        }                                                     \
        static void Set(DataT value)                          \
        {                                                     \
            data() |= value;                                  \
        }                                                     \
        static void Clear(DataT value)                        \
        {                                                     \
            data() &= ~value;                                 \
        }                                                     \
        static void Toggle(DataT value)                       \
        {                                                     \
            data() ^= value;                                  \
        }                                                     \
        static void DirSet(DataT value)                       \
        {                                                     \
            dir() |= value;                                   \
        }                                                     \
        static void DirClear(DataT value)                     \
        {                                                     \
            dir() &= ~value;                                  \
        }                                                     \
        static void DirToggle(DataT value)                    \
        {                                                     \
            dir() ^= value;                                   \
        }                                                     \
        static DataT PinRead()                                \
        {                                                     \
            return pin();                                     \
        }                                                     \
        enum                                                  \
        {                                                     \
            Id = ID                                           \
        };                                                    \
        enum                                                  \
        {                                                     \
            Width = sizeof(DataT) * 8                         \
        };                                                    \
    };

// Класс линии ввода вывода
template <class PORT, uint8_t PIN>
class TPin
{
public:
    // typedef PORT Port;
    enum
    {
        Number = PIN
    };

    static void Set()
    {
        PORT::Set(1 << PIN);
    }

    static void Set(uint8_t val)
    {
        if (val)
            Set();
        else
            Clear();
    }

    static void SetDir(uint8_t val)
    {
        if (val)
            SetDirWrite();
        else
            SetDirRead();
    }

    static void Clear()
    {
        PORT::Clear(1 << PIN);
    }

    static void On()
    {
        PORT::Set(1 << PIN);
    }

    static void Off()
    {
        PORT::Clear(1 << PIN);
    }

    static void Toggle()
    {
        PORT::Toggle(1 << PIN);
    }
    static void SetDirRead()
    {
        PORT::DirClear(1 << PIN);
    }
    static void SetDirWrite()
    {
        PORT::DirSet(1 << PIN);
    }
    static bool IsSet()
    {
        return PORT::PinRead() & (uint8_t)(1 << PIN);
    }
};