#include "internals.h"
#include <cbor.h>

static bool dumprecursive(CborValue *it, int nestingLevel)
{
    char indent[32];
    int idx = 0, lvl = nestingLevel;
 
    while (lvl--)
    {
        indent[idx++] = ' ';
        indent[idx++] = ' ';
    }
    indent[idx] = '\0';
 
    while (!cbor_value_at_end(it))
    {
        CborError err;
        CborType type = cbor_value_get_type(it);
 
        switch (type)
        {
        case CborArrayType:
        case CborMapType:
        {
            /* Recursive type. */
            CborValue recursed;
            LOG_ARG(LVL_INFO, "%s%s", indent,
                                  type == CborArrayType ? "Array[" : "Map[");
            err = cbor_value_enter_container(it, &recursed);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            err = dumprecursive(&recursed, nestingLevel + 1);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            err = cbor_value_leave_container(it, &recursed);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG_ARG(LVL_INFO, "%s]", indent);
            continue;
        }
 
        case CborIntegerType:
        {
            int val;
            err = cbor_value_get_int_checked(it, &val);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG_ARG(LVL_INFO, "%sInteger: %d", indent, val);
            break;
        }
 
        case CborByteStringType:
        {
            uint8_t data[64];
            size_t n = 64;
            err = cbor_value_copy_byte_string(it, data, &n, it);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG_ARG(LVL_INFO, "%sByteString: ", indent);
            LOG_ARG(LVL_INFO, data, n);
 
            continue;
        }
 
        case CborTextStringType:
        {
            char data[64];
            size_t n = 64;
            err = cbor_value_copy_text_string(it, data, &n, it);
            if (err != CborNoError)
            {
                /* Parse error. */
                return err;
            }
            LOG_ARG(LVL_INFO, "%sTextString: %s", indent, data);
            continue;
        }
 
        case CborTagType:
        {
            CborTag tag;
            cbor_value_get_tag(it, &tag); // can't fail
            LOG_ARG(LVL_INFO, "%sTag(%d)", indent, tag);
            break;
        }
 
        case CborSimpleType:
        {
            uint8_t sType;
            cbor_value_get_simple_type(it, &sType); // can't fail
            LOG_ARG(LVL_INFO, "%sSimple(%u)", indent, sType);
            break;
        }
 
        case CborNullType:
        {
            LOG_ARG(LVL_INFO, "%sNull", indent);
            break;
        }
 
        case CborUndefinedType:
        {
            LOG_ARG(LVL_INFO, "%sUndefined", indent);
            break;
        }
 
        case CborBooleanType:
        {
            bool val;
            cbor_value_get_boolean(it, &val); // can't fail
            LOG_ARG(LVL_INFO, "%sBoolean: %s", indent, val ? "true" : "false");
            break;
        }
 
        case CborDoubleType:
        case CborFloatType:
        case CborHalfFloatType:
        {
            /* Not managed. */
            LOG_ARG(LVL_WARNING, "%sFloat: (not managed)", indent);
            break;
        }
 
        case CborInvalidType:
            /* Can't happen. */
            LOG_ARG(LVL_INFO, "%sInvalidType", indent);
            break;
        }
 
        err = cbor_value_advance_fixed(it);
        if (err != CborNoError)
        {
            /* Parse error. */
            return err;
        }
    }
    return CborNoError;
}

int cbor_parse(const uint8_t * buffer,
              size_t bufferLen,
              lwm2m_data_t ** dataP)
{
    CborParser parser;
    CborValue value;
    CborError err;

    err = cbor_parser_init(buffer, bufferLen, 0, &parser, &value);

    if (err == CborNoError)
    {
        err = cbor_value_validate(&value, CborValidateStrictMode);
    }
 
    if (err == CborNoError)
    {
        err = dumprecursive(&value, 0);
    }
 
    if (err != CborNoError)
    {
        LOG_ARG(LVL_ERROR, "CBOR parsing failure at offset %ld: %s",
                       value.ptr - m_cbor_buffer,
                       cbor_error_string(err));
    }

    return 0;
}

int cbor_serialize(bool isResourceInstance, 
                  int size,
                  lwm2m_data_t * dataP,
                  uint8_t ** bufferP)
{
    //Encode
    uint8_t buff[100];

    CborEncoder encoder;

    cbor_encoder_init(&encoder, buff, sizeof(buff), 0);


    return 0;
}
