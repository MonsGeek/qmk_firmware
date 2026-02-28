ENCODER_MAP_ENABLE = yes
VIA_ENABLE = yes

# Enable RGB record functionality for default keymap only
RGB_RECORD_ENABLE = yes

# Define the preprocessor flag
OPT_DEFS += -DRGB_RECORD_ENABLE

SRC += ./rgb_record/rgb_record.c
