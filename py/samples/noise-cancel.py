
# pip install torchaudio speechbrain

import torchaudio
from speechbrain.inference.enhancement import SpectralMaskEnhancement

# 1. Load the model from the cloud (happens automatically)
enhance_model = SpectralMaskEnhancement.from_hparams(
    source="speechbrain/metricgan-plus-voicebank", 
    savedir="pretrained_models"
)

# 2. Run your echoey WAV file through the AI
# It automatically detects the room reflections and masks them out
enhance_model.enhance_file("intermed.wav", "cleaned_try1.wav")


