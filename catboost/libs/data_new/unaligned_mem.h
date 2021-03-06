#pragma once

#include <catboost/libs/helpers/exception.h>
#include <catboost/libs/helpers/maybe_owning_array_holder.h>

#include <util/generic/array_ref.h>
#include <util/generic/type_name.h>
#include <util/system/unaligned_mem.h>

#include <cstring>
#include <cstdlib>


namespace NCB {

    template <class T, unsigned Align = alignof(T)>
    class TUnalignedArrayBuf {
    public:
        TUnalignedArrayBuf(void* begin, size_t sizeInBytes, bool sizeWithoutPadding = true)
            : Begin(begin)
            , SizeInBytes(sizeWithoutPadding ? sizeInBytes : sizeInBytes - sizeInBytes % Align)
        {
            CB_ENSURE_INTERNAL(
                !sizeWithoutPadding || !(SizeInBytes % sizeof(T)),
                "sizeWithoutPadding does not correspond to size of array of type " << TypeName<T>()
            );
        }

        size_t GetSize() const {
            return SizeInBytes / sizeof(T);
        }

        void WriteTo(TArrayRef<T>* dst) const {
            CB_ENSURE_INTERNAL(
                dst->size() == GetSize(),
                "TUnalignedArrayBuf::WriteTo: Wrong destination array size"
            );
            memcpy(dst->Data(), Begin, SizeInBytes);
        }

        void WriteTo(TVector<T>* dst) const {
            dst->yresize(GetSize());
            memcpy(dst->data(), Begin, SizeInBytes);
        }

        // allocates and copies only if Begin is unaligned
        TMaybeOwningArrayHolder<T> GetAlignedData() const {
            if (reinterpret_cast<size_t>(Begin) % Align) {
                TVector<T> alignedData;
                WriteTo(&alignedData);
                return TMaybeOwningArrayHolder<T>::CreateOwning(std::move(alignedData));
            } else {
                return TMaybeOwningArrayHolder<T>::CreateNonOwning(TArrayRef<T>((T*)Begin, GetSize()));
            }
        }

        TUnalignedMemoryIterator<T, Align> GetIterator() const {
            return TUnalignedMemoryIterator<T, Align>(Begin, SizeInBytes);
        }


    private:
        void* Begin;
        size_t SizeInBytes;
    };

}
