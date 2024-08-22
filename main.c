#include <stdio.h>
#include <stdlib.h>

#define HEAP_CAPACITY 640000
#define CHUNK_LIST_CAP 1024

typedef struct {
  void *start;
  size_t size;
} Chunk;

typedef struct {
  size_t count;
  Chunk chunks[CHUNK_LIST_CAP];
} Chunk_List;

char heap[HEAP_CAPACITY] = {0};

Chunk_List alloced_chunks = {0};
Chunk_List freed_chunks = {
    .count = 1,
    .chunks =
        {
            [0] = {.start = heap, .size = sizeof(heap)},
        },
};

Chunk_List tmp_chunk = {0};

void chunk_list_insert(Chunk_List *list, void *start, size_t size) {
  list->chunks[list->count].start = start;
  list->chunks[list->count].size = size;

  for (int i = list->count;
       i > 0 && list->chunks[i].start < list->chunks[i - 1].start; i--) {
    const Chunk t = list->chunks[i];
    list->chunks[i] = list->chunks[i - 1];
    list->chunks[i - 1] = t;
  }

  list->count++;
}

int chunk_start_compar(const void *a, const void *b) {
  const Chunk *a_chunk = a;
  const Chunk *b_chunk = b;

  return a_chunk->start - b_chunk->start;
}

int chunk_list_find(Chunk_List *list, void *start) {
  const Chunk key = {
      .start = start,
  };

  Chunk *result = bsearch(&key, list->chunks, list->count,
                          sizeof(list->chunks[0]), chunk_start_compar);

  if (result != NULL) {
    return result - list->chunks;
  } else {
    return -1;
  }
}

void chunk_list_remove(Chunk_List *list, size_t index) {
  for (size_t i = index; i < list->count - 1; i++) {
    list->chunks[i] = list->chunks[i + 1];
  }

  list->count--;
}

void *heap_alloc(size_t size) {
  if (size > 0) {
    for (size_t i = 0; i < freed_chunks.count; i++) {
      const Chunk chunk = freed_chunks.chunks[i];
      if (chunk.size >= size) {
        chunk_list_remove(&freed_chunks, i);

        const size_t tail_size = chunk.size - size;
        chunk_list_insert(&alloced_chunks, chunk.start, size);

        if (tail_size > 0) {
          chunk_list_insert(&freed_chunks, chunk.start + size, tail_size);
        }

        return chunk.start;
      }
    }
  }

  return NULL;
}

void chunk_list_merge(Chunk_List *src) {
  Chunk_List dst = {0};
  size_t merge_size = 0;
  void *merge_start = NULL;

  for (int i = 0; i < src->count; i++) {
    const Chunk chunk = src->chunks[i];
    if (merge_start == NULL || chunk.start == merge_start + merge_size) {
      if (merge_start == NULL) {
        merge_start = chunk.start;
      }

      merge_size += chunk.size;
    } else {
      chunk_list_insert(&dst, merge_start, merge_size);
      merge_start = chunk.start;
      merge_size = chunk.size;
    }
  }

  if (merge_start != NULL) {
    chunk_list_insert(&dst, merge_start, merge_size);
  }

  *src = dst;
}

void heap_free(void *ptr) {
  if (ptr != NULL) {
    const int index = chunk_list_find(&alloced_chunks, ptr);
    if (index != -1) {
      chunk_list_insert(&freed_chunks, alloced_chunks.chunks[index].start,
                        alloced_chunks.chunks[index].size);
      chunk_list_remove(&alloced_chunks, (size_t)index);

      chunk_list_merge(&freed_chunks);
    }
  }
}

void chunk_list_dump(Chunk_List *list) {
  printf("Chunks (%zu):\n", list->count);
  for (size_t i = 0; i < list->count; i++) {
    printf("pointer = %p, size = %zu\n", list->chunks[i].start,
           list->chunks[i].size);
  }
}
int main() {
  void *ptr = heap_alloc(123);
  void *ptr1 = heap_alloc(420);
  void *ptr2 = heap_alloc(69);

  heap_free(ptr);
  heap_free(ptr1);
  heap_free(ptr2);

  chunk_list_dump(&freed_chunks);

  return 0;
}
