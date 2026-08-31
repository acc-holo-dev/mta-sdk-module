# Исходники собираются рекурсивным glob-ом: чтобы добавить функцию, не
# нужно трогать файлы сборки — просто положи .cpp куда угодно под src/.
# CONFIGURE_DEPENDS перезапускает glob при изменении набора файлов.
file(GLOB_RECURSE SDK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
