/*****************************************************************************
Copyright (c) 2019 Reed A. Cartwright, PhD <reed@cartwrig.ht>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*****************************************************************************/
#pragma once
#ifndef SOFOS_VCF_HPP
#define SOFOS_VCF_HPP

#include <htslib/vcf.h>
#include <htslib/vcfutils.h>

#include <memory>

// The *_free_t classes are used enable RAII on pointers created by htslib.
struct buffer_free_t {
    void operator()(void* ptr) const {
        free(ptr);
    }
};
struct file_free_t {
    void operator()(void* ptr) const {
        vcf_close(reinterpret_cast<vcfFile*>(ptr));
    }
};
struct header_free_t {
    void operator()(void* ptr) const {
        bcf_hdr_destroy(reinterpret_cast<bcf_hdr_t*>(ptr));
    }
};
struct bcf_free_t {
    void operator()(void* ptr) const {
        bcf_destroy(reinterpret_cast<bcf1_t*>(ptr));
    }
};

// Templates and functions for handling buffers used by htslib
template<typename T>
struct buffer_t {
    std::unique_ptr<T[],buffer_free_t> data;
    int capacity;
};

template<typename T>
inline
buffer_t<T> make_buffer(int sz) {
    void *p = std::malloc(sizeof(T)*sz);
    if(p == nullptr) {
        throw std::bad_alloc{};
    }
    return {std::unique_ptr<T[],buffer_free_t>{reinterpret_cast<T*>(p)}, sz};
}

// htslib may call realloc on our pointer. When using a managed buffer,
// we need to check to see if it needs to be updated.
inline
int get_info_string(const bcf_hdr_t *header, bcf1_t *record,
    const char *tag, buffer_t<char>* buffer)
{
    char *p = buffer->data.get();
    int n = bcf_get_info_string(header, record, tag, &p, &buffer->capacity);
    if(n == -4) {
        throw std::bad_alloc{};
    } else if(p != buffer->data.get()) {
        // update pointer
        buffer->data.release();
        buffer->data.reset(p);
    }
    return n;
}

inline
int get_info_int32(const bcf_hdr_t *header, bcf1_t *record,
    const char *tag, buffer_t<int32_t>* buffer)
{
    int32_t *p = buffer->data.get();
    int n = bcf_get_info_int32(header, record, tag, &p, &buffer->capacity);
    if(n == -4) {
        throw std::bad_alloc{};
    } else if(p != buffer->data.get()) {
        // update pointer
        buffer->data.release();
        buffer->data.reset(p);
    }
    return n;
}

inline
int get_format_float(const bcf_hdr_t *header, bcf1_t *record,
    const char *tag, buffer_t<float>* buffer)
{
    float *p = buffer->data.get();
    int n = bcf_get_format_float(header, record, tag, &p, &buffer->capacity);
    if(n == -4) {
        throw std::bad_alloc{};
    } else if(p != buffer->data.get()) {
        // update pointer
        buffer->data.release();
        buffer->data.reset(p);
    }
    return n;
}

inline
int get_genotypes(const bcf_hdr_t *header, bcf1_t *record,
    buffer_t<int32_t>* buffer)
{
    int32_t *p = buffer->data.get();
    int n = bcf_get_genotypes(header, record, &p, &buffer->capacity);
    if(n == -4) {
        throw std::bad_alloc{};
    } else if(p != buffer->data.get()) {
        // update pointer
        buffer->data.release();
        buffer->data.reset(p);
    }
    return n;
}

// an allele is missing if its value is '.', 'N', or 'n'.
inline
bool is_allele_missing(const char* a) {
    if(a == nullptr) {
        return true;
    }
    if(a[0] == '\0') {
        return true;
    }
    if((a[0] == '.' || a[0] == 'N' || a[0] == 'n') && a[1] == '\0') {
        return true;
    }
    return false;
}

// determine if the reference allele is missing
inline
bool is_ref_missing(bcf1_t* record) {
    if(record->n_allele == 0) {
        return true;
    }
    bcf_unpack(record, BCF_UN_STR);
    return is_allele_missing(record->d.allele[0]);
}

#endif // SOFOS_VCF_HPP
