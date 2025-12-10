export default {
	rgb: {
		'header-text': 'RGB LED Configuration',
		'data-pin-label': 'Data GPIO Pin (-1 for disabled)',
		'led-format-label': 'LED Format',
		'led-layout-label': 'LED Layout',
		'leds-per-button-label': 'LEDs Per Button',
		'led-brightness-maximum-label': 'Max Brightness',
		'led-brightness-steps-label': 'Brightness Steps',
	},
	player: {
		'header-text': 'Player LEDs',
		'pwm-sub-header-text':
			'For PWM LEDs, set each LED to a dedicated GPIO pin.',
		'rgb-sub-header-text':
			'For RGB LEDs, the indexes must be after the last LED button defined in <1>RGB LED Button Order</1> section and likely <3>starts at index {{rgbLedStartIndex}}</3>.',
		'pled-type-label': 'Player LED Type',
		'pled-type-off': 'Off',
		'pled-type-pwm': 'PWM',
		'pled-type-rgb': 'RGB',
		'pled-color-label': 'RGB PLED Color',
	},
	case: {
		'header-text': 'Case RGB LEDs',
		'sub-header-text':
			'For Case RGB LEDs, set a starting index and the case RGB count. The index must be after the last LED button defined but can be before or after the player LEDs and turbo LED.',
		'case-index-label': 'RGB LED Index',
		'case-count-label': 'RGB LED Count',
		'case-type-label': 'Color Type',
		'case-type-off': 'Off',
		'case-type-ambient': 'Ambient',
		'case-type-linked': 'Linked',
	},
	'pled-pin-label': 'PLED #{{pin}} GPIO Pin',
	'pled-index-label': 'PLED #{{index}} Index',
        'rgb-order': {
                'header-text': 'RGB LED Button Order',
                'sub-header-text':
                        'Here you can define which buttons have RGB LEDs and in what order they run from the control board. This is required for certain LED animations and static theme support.',
                'sub-header1-text':
                        'Drag and drop list items to assign and reorder the RGB LEDs.',
                'available-header-text': 'Available Buttons',
                'assigned-header-text': 'Assigned Buttons',
        },
        'turn-off-when-suspended': 'Turn Off When Suspended',
        grid: {
                'header-text': 'Grid Gradient Preset',
                'sub-header-text':
                        'Configure the built-in grid gradient animation preset. These colors and indices are stored on the controller and can be cycled to with the normal LED animation hotkeys.',
                'gradient-a': 'Gradient Color A',
                'gradient-b': 'Gradient Color B',
                'button-press': 'Button Press Color',
                'lever-normal': 'Lever Normal Color',
                'lever-press': 'Lever Press Color',
                'case-normal': 'Case Normal Color',
                'case-press': 'Case Lever Press Color',
                'speed-label': 'Gradient Speed',
                'pause-label': 'Gradient Pause',
                'speed-slow': 'Slow',
                'speed-normal': 'Normal',
                'speed-fast': 'Fast',
                'pause-0': '0 seconds',
                'pause-1': '1 second',
                'pause-2': '2 seconds',
                'pause-3': '3 seconds',
                'case-help-text':
                        'Case LED indices are offsets from the Case RGB index. Leave entries blank to skip LEDs for that direction.',
                'case-up-label': 'Case LEDs for Up',
                'case-down-label': 'Case LEDs for Down',
                'case-right-label': 'Case LEDs for Right',
                'case-left-label': 'Case LEDs for Left',
        },
};
