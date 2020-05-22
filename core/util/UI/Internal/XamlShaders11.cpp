#include "pch.h"
#include "Data/Data.h"
#include "UI/Internal/XamlShaders11.h"

static void FastLZMove(uint8_t* dest, const uint8_t* src, uint32_t count)
{
    if (count > 4 && dest >= src + count)
    {
        std::memmove(dest, src, count);
    }
    else
    {
        switch (count)
        {
        default:
            do { *dest++ = *src++; } while (--count);
            break;
        case 3:
            *dest++ = *src++;
        case 2:
            *dest++ = *src++;
        case 1:
            *dest++ = *src++;
        case 0:
            break;
        }
    }
}

ff::ComPtr<ff::IDataVector> ff::DecompressShaders()
{
    const uint32_t MAX_L2_DISTANCE = 8191;
    const uint32_t MAGIC = 0x4b50534e;

    struct Header
    {
        uint32_t magic;
        uint32_t size;
    };

    Header* header = (Header*)ff::Shaders;
    assert(header->magic == MAGIC);

    ff::Vector<BYTE> output;
    output.Resize(header->size);

    const uint8_t* ip = (const uint8_t*)ff::Shaders + sizeof(Header);
    const uint8_t* ip_limit = ip + sizeof(ff::Shaders) - sizeof(Header);
    const uint8_t* ip_bound = ip_limit - 2;
    uint8_t* op = output.Data();
    uint8_t* op_limit = op + header->size;
    uint32_t ctrl = (*ip++) & 31;

    while (1)
    {
        if (ctrl >= 32)
        {
            uint32_t len = (ctrl >> 5) - 1;
            uint32_t ofs = (ctrl & 31) << 8;
            const uint8_t* ref = op - ofs - 1;

            uint8_t code;
            if (len == 7 - 1)
            {
                do
                {
                    assert(ip <= ip_bound);
                    code = *ip++;
                    len += code;
                } while (code == 255);
            }

            code = *ip++;
            ref -= code;
            len += 3;

            /* match from 16-bit distance */
            if (NS_UNLIKELY(code == 255))
            {
                if (NS_LIKELY(ofs == (31 << 8)))
                {
                    assert(ip < ip_bound);
                    ofs = (*ip++) << 8;
                    ofs += *ip++;
                    ref = op - ofs - MAX_L2_DISTANCE - 1;
                }
            }

            assert(op + len <= op_limit);
            assert(ref >= output.Data());
            FastLZMove(op, ref, len);
            op += len;
        }
        else
        {
            ctrl++;
            assert(op + ctrl <= op_limit);
            assert(ip + ctrl <= ip_limit);
            memcpy(op, ip, ctrl);
            ip += ctrl;
            op += ctrl;
        }

        if (NS_UNLIKELY(ip >= ip_limit)) break;
        ctrl = *ip++;
    }

    assert((int)(op - output.Data()) == header->size);

    return ff::CreateDataVector(std::move(output));
}
