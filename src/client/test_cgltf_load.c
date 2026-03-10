#include "../../libs/cgltf/cgltf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s <gltf_file>\n", argv[0]);
        return 1;
    }

    const char* filepath = argv[1];

    // Read file into memory
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open %s\n", filepath);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void* file_data = malloc(file_size);
    if (!file_data) {
        fprintf(stderr, "Failed to allocate memory for %s\n", filepath);
        fclose(fp);
        return 1;
    }
    fread(file_data, 1, file_size, fp);
    fclose(fp);

    cgltf_options options = {0};
    cgltf_data* data = NULL;

    // Parse from memory
    cgltf_result parse_result = cgltf_parse(&options, file_data, file_size, &data);
    if (parse_result != cgltf_result_success) {
        fprintf(stderr, "Failed to parse glTF from memory: %s (error %d)\n", filepath, parse_result);
        free(file_data);
        return 1;
    }

    // Load buffers from memory (should be embedded in GLB)
    cgltf_result load_buffers_result = cgltf_load_buffers(&options, data, NULL); // Pass NULL for filepath for embedded buffers
    if (load_buffers_result != cgltf_result_success) {
        fprintf(stderr, "Failed to load glTF buffers from memory: %s (error %d)\n", filepath, load_buffers_result);
        cgltf_free(data);
        free(file_data);
        return 1;
    }

    printf("Successfully loaded glTF file: %s\n", filepath);
    printf("  Meshes: %zu\n", data->meshes_count);
    printf("  Nodes: %zu\n", data->nodes_count);
    printf("  Animations: %zu\n", data->animations_count);
    printf("  Skins: %zu\n", data->skins_count);

    cgltf_free(data);
    free(file_data);

    return 0;
}
