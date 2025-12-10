import { useContext, useEffect, useState } from 'react';
import Button from 'react-bootstrap/Button';
import Col from 'react-bootstrap/Col';
import Form from 'react-bootstrap/Form';
import Row from 'react-bootstrap/Row';

import DraggableListGroup from '../Components/DraggableListGroup';

import { Formik, useFormikContext } from 'formik';
import * as yup from 'yup';
import orderBy from 'lodash/orderBy';
import { Trans, useTranslation } from 'react-i18next';

import { AppContext } from '../Contexts/AppContext';
import ColorPicker from '../Components/ColorPicker';
import Section from '../Components/Section';
import FormControl from '../Components/FormControl';
import FormSelect from '../Components/FormSelect';
import { getButtonLabels } from '../Data/Buttons';
import LEDColors from '../Data/LEDColors';
import { hexToInt } from '../Services/Utilities';
import WebApi from '../Services/WebApi';
import { BUTTON_LAYOUTS } from '../Data/Buttons';

const LED_FORMATS = [
	{ label: 'GRB', value: 0 },
	{ label: 'RGB', value: 1 },
	{ label: 'GRBW', value: 2 },
	{ label: 'RGBW', value: 3 },
];

const PLED_LABELS = [
	{ 0: 'PLED #1 Pin', 1: 'PLED #1 Index' },
	{ 0: 'PLED #2 Pin', 1: 'PLED #2 Index' },
	{ 0: 'PLED #3 Pin', 1: 'PLED #3 Index' },
	{ 0: 'PLED #4 Pin', 1: 'PLED #4 Index' },
];

const CASE_TYPE = [
        { value: -1, label: 'Off' },
        { value: 0, label: 'Ambient' },
        { value: 1, label: 'Linked' },
];

const GRID_GRADIENT_SPEED = [
        { value: 0, label: 'slow' },
        { value: 1, label: 'normal' },
        { value: 2, label: 'fast' },
];

const GRID_GRADIENT_PAUSE = [
        { value: 0, label: '0s' },
        { value: 1, label: '1s' },
        { value: 2, label: '2s' },
        { value: 3, label: '3s' },
];

const defaultValue = {
        brightnessMaximum: 255,
        brightnessSteps: 5,
        dataPin: -1,
        ledFormat: 0,
        ledLayout: 0,
        ledsPerButton: 2,
        pledType: -1,
        pledPin1: -1,
        pledPin2: -1,
        pledPin3: -1,
        pledPin4: -1,
        pledIndex1: -1,
        pledIndex2: -1,
        pledIndex3: -1,
        pledIndex4: -1,
        pledColor: '#00ff00',
        caseRGBType: 0,
        caseRGBIndex: -1,
        caseRGBCount: 0,
        ledButtonMap: {},
        gridGradientColorA: '#0000ff',
        gridGradientColorB: '#ff00ff',
        gridButtonPressColor: '#ffffff',
        gridGradientSpeed: 1,
        gridGradientPause: 0,
        gridLeverNormalColor: '#202020',
        gridLeverPressColor: '#00ffff',
        gridCaseNormalColor: '#000000',
        gridCaseLeverPressColor: '#ff8000',
        gridCaseUpIndices: ['', '', '', ''],
        gridCaseDownIndices: ['', '', '', ''],
        gridCaseRightIndices: ['', '', '', ''],
        gridCaseLeftIndices: ['', '', '', ''],
};

const schema = yup.object().shape({
	brightnessMaximum: yup
		.number()
		.required()
		.positive()
		.integer()
		.min(0)
		.max(255)
		.label('Max Brightness'),
	brightnessSteps: yup
		.number()
		.required()
		.positive()
		.integer()
		.min(1)
		.max(10)
		.label('Brightness Steps'),
	dataPin: yup.number().required().checkUsedPins(),
	ledFormat: yup
		.number()
		.required()
		.positive()
		.integer()
		.min(0)
		.max(3)
		.label('LED Format'),
	ledLayout: yup
		.number()
		.required()
		.positive()
		.integer()
		.min(0)
		.max(38)
		.label('LED Layout'),
	ledsPerButton: yup
		.number()
		.required()
		.positive()
		.integer()
		.min(1)
		.label('LEDs Per Pixel'),
	pledType: yup.number().required().label('Player LED Type'),
	pledColor: yup.string().label('RGB Player LEDs').validateColor(),
	pledPin1: yup
		.number()
		.label('PLED 1')
		.validatePinWhenEqualTo('pledPins1', 'pledType', 0),
	pledPin2: yup
		.number()
		.label('PLED 2')
		.validatePinWhenEqualTo('pledPins2', 'pledType', 0),
	pledPin3: yup
		.number()
		.label('PLED 3')
		.validatePinWhenEqualTo('pledPins3', 'pledType', 0),
	pledPin4: yup
		.number()
		.label('PLED 4')
		.validatePinWhenEqualTo('pledPins4', 'pledType', 0),
	pledIndex1: yup
		.number()
		.label('PLED Index 1')
		.validateMinWhenEqualTo('pledType', 1, 0),
	pledIndex2: yup
		.number()
		.label('PLED Index 2')
		.validateMinWhenEqualTo('pledType', 1, 0),
	pledIndex3: yup
		.number()
		.label('PLED Index 3')
		.validateMinWhenEqualTo('pledType', 1, 0),
	pledIndex4: yup
		.number()
		.label('PLED Index 4')
		.validateMinWhenEqualTo('pledType', 1, 0),
	turnOffWhenSuspended: yup.number().label('Turn Off When Suspended'),
	caseRGBType: yup.number().required().label('Case RGB Type'),
        caseRGBCount: yup
                .number()
                .required()
                .positive()
                .integer()
                .min(0)
                .max(100)
                .label('Case RGB Count'),
        caseRGBIndex: yup.number().label('Case RGB Index').min(-1).max(100),
        ledButtonMap: yup.object(),
        gridGradientColorA: yup.string().label('Grid Color A').validateColor(),
        gridGradientColorB: yup.string().label('Grid Color B').validateColor(),
        gridButtonPressColor: yup.string().label('Grid Press Color').validateColor(),
        gridGradientSpeed: yup.number().label('Grid Speed').min(0).max(2),
        gridGradientPause: yup.number().label('Grid Pause').min(0).max(3),
        gridLeverNormalColor: yup.string().label('Lever Color').validateColor(),
        gridLeverPressColor: yup.string().label('Lever Press Color').validateColor(),
        gridCaseNormalColor: yup.string().label('Case Color').validateColor(),
        gridCaseLeverPressColor: yup.string().label('Case Press Color').validateColor(),
        gridCaseUpIndices: yup.array().of(yup.number().integer().min(0).nullable()),
        gridCaseDownIndices: yup.array().of(yup.number().integer().min(0).nullable()),
        gridCaseRightIndices: yup.array().of(yup.number().integer().min(0).nullable()),
        gridCaseLeftIndices: yup.array().of(yup.number().integer().min(0).nullable()),
});

const createDataSource = (ledButtonMap, buttonLabelType, swapTpShareLabels) => {
	let available = {};
	let assigned = {};

	Object.keys(ledButtonMap).forEach((p) => {
		if (ledButtonMap[p] === null) available[p] = ledButtonMap[p];
		else assigned[p] = ledButtonMap[p];
	});

	const dataSources = [
		getLedButtons(buttonLabelType, available, true, swapTpShareLabels),
		getLedButtons(buttonLabelType, assigned, true, swapTpShareLabels),
	];

	return dataSources;
};

const getLedButtons = (buttonLabels, map, excludeNulls, swapTpShareLabels) => {
	const current_buttons = getButtonLabels(buttonLabels, swapTpShareLabels);
	return orderBy(
		Object.keys(current_buttons)
			.filter((p) => p !== 'label' && p !== 'value')
			.filter((p) => (excludeNulls ? map[p] > -1 : true))
			.map((p) => {
				return { id: p, label: current_buttons[p], value: map[p] };
			}),
		'value',
	);
};

const createLedMap = (ledButtons, clear) => {
        if (!ledButtons) return;
        return ledButtons.reduce(
                (acc, btn, index) => ({ ...acc, [btn.id]: clear ? null : index }),
                {},
        );
};

const ensureIndexArray = (arr) => {
        const padded = [...(arr || [])];
        while (padded.length < 4) padded.push('');
        return padded.slice(0, 4).map((v) => (v === null || typeof v === 'undefined' ? '' : v));
};

const sanitizeIndexArray = (arr) =>
        (arr || [])
                .map((value) => {
                        if (value === '' || value === null || typeof value === 'undefined') return null;
                        const parsed = parseInt(value);
                        return Number.isNaN(parsed) || parsed < 0 ? null : parsed;
                })
                .filter((v) => v !== null);

const FormContext = ({
	buttonLabelType,
	ledButtonMap,
	swapTpShareLabels,
	setDataSources,
}) => {
	const { setValues } = useFormikContext();
	const { setLoading } = useContext(AppContext);

	useEffect(() => {
		async function fetchData() {
                        const data = await WebApi.getLedOptions(setLoading);
                        const dataSources = createDataSource(
                                data.ledButtonMap,
                                buttonLabelType,
                                swapTpShareLabels,
                        );
                        setDataSources(dataSources);
                        setValues({
                                ...data,
                                gridCaseUpIndices: ensureIndexArray(data.gridCaseUpIndices),
                                gridCaseDownIndices: ensureIndexArray(data.gridCaseDownIndices),
                                gridCaseRightIndices: ensureIndexArray(data.gridCaseRightIndices),
                                gridCaseLeftIndices: ensureIndexArray(data.gridCaseLeftIndices),
                        });
                }
                fetchData();
        }, []);

	useEffect(() => {
		const dataSources = createDataSource(
			ledButtonMap,
			buttonLabelType,
			swapTpShareLabels,
		);
		setDataSources(dataSources);
	}, [buttonLabelType, swapTpShareLabels]);

	return null;
};

export default function LEDConfigPage() {
        const { buttonLabels, updateUsedPins } = useContext(AppContext);
        const [saveMessage, setSaveMessage] = useState('');
        const [dataSources, setDataSources] = useState([[], []]);
        const [colorPickerTarget, setColorPickerTarget] = useState(null);
        const [colorPickerName, setColorPickerName] = useState(null);
        const [showPicker, setShowPicker] = useState(false);
        const [rgbLedStartIndex, setRgbLedStartIndex] = useState(0);

	const { buttonLabelType, swapTpShareLabels } = buttonLabels;

	const { t } = useTranslation('');

	// Translate PLED labels
	PLED_LABELS.map((p, n) => {
		p[0] = t(`LedConfig:pled-pin-label`, { pin: ++n });
		p[1] = t(`LedConfig:pled-index-label`, { index: n });
	});

        CASE_TYPE[0].label = t(`LedConfig:case.case-type-off`);
        CASE_TYPE[1].label = t(`LedConfig:case.case-type-ambient`);
        CASE_TYPE[2].label = t(`LedConfig:case.case-type-linked`);

        GRID_GRADIENT_SPEED[0].label = t(`LedConfig:grid.speed-slow`);
        GRID_GRADIENT_SPEED[1].label = t(`LedConfig:grid.speed-normal`);
        GRID_GRADIENT_SPEED[2].label = t(`LedConfig:grid.speed-fast`);

        GRID_GRADIENT_PAUSE[0].label = t(`LedConfig:grid.pause-0`);
        GRID_GRADIENT_PAUSE[1].label = t(`LedConfig:grid.pause-1`);
        GRID_GRADIENT_PAUSE[2].label = t(`LedConfig:grid.pause-2`);
        GRID_GRADIENT_PAUSE[3].label = t(`LedConfig:grid.pause-3`);

	const ledOrderChanged = (setFieldValue, ledOrderArrays, ledsPerButton) => {
		if (ledOrderArrays.length === 2) {
			setRgbLedStartIndex(ledOrderArrays[1].length * (ledsPerButton || 0));
			setFieldValue('ledButtonMap', {
				...createLedMap(ledOrderArrays[0], true),
				...createLedMap(ledOrderArrays[1], false),
			});
		}
	};

        const ledsPerButtonChanged = (e, handleChange) => {
                const ledsPerButton = parseInt(e.target.value);
                setRgbLedStartIndex(dataSources[1].length * (ledsPerButton || 0));
                handleChange(e);
        };

        const toggleColorPicker = (name, e) => {
                e.stopPropagation();
                setColorPickerName(name);
                setColorPickerTarget(e.target);
                setShowPicker(name === colorPickerName ? !showPicker : true);
        };

        const onSuccess = async (values) => {
                const data = {
                        ...values,
                        pledColor: hexToInt(values.pledColor || '#000000'),
                        gridGradientColorA: hexToInt(values.gridGradientColorA || '#000000'),
                        gridGradientColorB: hexToInt(values.gridGradientColorB || '#000000'),
                        gridButtonPressColor: hexToInt(values.gridButtonPressColor || '#000000'),
                        gridLeverNormalColor: hexToInt(values.gridLeverNormalColor || '#000000'),
                        gridLeverPressColor: hexToInt(values.gridLeverPressColor || '#000000'),
                        gridCaseNormalColor: hexToInt(values.gridCaseNormalColor || '#000000'),
                        gridCaseLeverPressColor: hexToInt(values.gridCaseLeverPressColor || '#000000'),
                        gridCaseUpIndices: sanitizeIndexArray(values.gridCaseUpIndices),
                        gridCaseDownIndices: sanitizeIndexArray(values.gridCaseDownIndices),
                        gridCaseRightIndices: sanitizeIndexArray(values.gridCaseRightIndices),
                        gridCaseLeftIndices: sanitizeIndexArray(values.gridCaseLeftIndices),
                };

		const success = await WebApi.setLedOptions(data);
		if (success) updateUsedPins();

		// Need to recreate the DraggableList data source after save
		const dataSources = createDataSource(
			data.ledButtonMap,
			buttonLabelType,
			swapTpShareLabels,
		);
		setDataSources(dataSources);

		setSaveMessage(
			success
				? t('Common:saved-success-message')
				: t('Common:saved-error-message'),
		);
	};

	const onSubmit = (e, handleSubmit) => {
		e.preventDefault();
		setSaveMessage('');
		handleSubmit();
	};

	return (
		<Formik
			validationSchema={schema}
			onSubmit={onSuccess}
			initialValues={defaultValue}
		>
			{({
				handleSubmit,
				handleChange,
				handleBlur,
				values,
				errors,
				setFieldValue,
			}) => (
				<Form noValidate onSubmit={(e) => onSubmit(e, handleSubmit)}>
					<Section title={t('LedConfig:rgb.header-text')}>
						<Row>
							<FormControl
								type="number"
								label={t('LedConfig:rgb.data-pin-label')}
								name="dataPin"
								className="form-control-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.dataPin}
								error={errors.dataPin}
								isInvalid={errors.dataPin}
								onChange={handleChange}
								min={-1}
								max={29}
							/>
							<FormSelect
								label={t('LedConfig:rgb.led-format-label')}
								name="ledFormat"
								className="form-select-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.ledFormat}
								error={errors.ledFormat}
								isInvalid={errors.ledFormat}
								onChange={(e) =>
									setFieldValue('ledFormat', parseInt(e.target.value))
								}
							>
								{LED_FORMATS.map((o, i) => (
									<option key={`ledFormat-option-${i}`} value={o.value}>
										{o.label}
									</option>
								))}
							</FormSelect>
							<FormSelect
								label={t('LedConfig:rgb.led-layout-label')}
								name="ledLayout"
								className="form-select-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.ledLayout}
								error={errors.ledLayout}
								isInvalid={errors.ledLayout}
								onChange={(e) =>
									setFieldValue('ledLayout', parseInt(e.target.value))
								}
							>
								{BUTTON_LAYOUTS.map((o, i) => (
									<option key={`ledLayout-option-${i}`} value={o.value}>
										{o.label}
									</option>
								))}
							</FormSelect>
						</Row>
						<Row>
							<FormControl
								type="number"
								label={t('LedConfig:rgb.leds-per-button-label')}
								name="ledsPerButton"
								className="form-control-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.ledsPerButton}
								error={errors.ledsPerButton}
								isInvalid={errors.ledsPerButton}
								onChange={(e) => ledsPerButtonChanged(e, handleChange)}
								min={1}
							/>
							<FormControl
								type="number"
								label={t('LedConfig:rgb.led-brightness-maximum-label')}
								name="brightnessMaximum"
								className="form-control-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.brightnessMaximum}
								error={errors.brightnessMaximum}
								isInvalid={errors.brightnessMaximum}
								onChange={handleChange}
								min={0}
								max={255}
							/>
							<FormControl
								type="number"
								label={t('LedConfig:rgb.led-brightness-steps-label')}
								name="brightnessSteps"
								className="form-control-sm"
								groupClassName="col-sm-4 mb-3"
								value={values.brightnessSteps}
								error={errors.brightnessSteps}
								isInvalid={errors.brightnessSteps}
								onChange={handleChange}
								min={1}
								max={10}
							/>
							<div className="col-sm-3">
								<Form.Check
									label={t('LedConfig:turn-off-when-suspended')}
									type="switch"
									name="turnOffWhenSuspended"
									isInvalid={false}
									checked={Boolean(values.turnOffWhenSuspended)}
									onChange={(e) => {
										setFieldValue(
											'turnOffWhenSuspended',
											e.target.checked ? 1 : 0,
										);
									}}
								/>
							</div>
						</Row>
					</Section>
					<Section title={t('LedConfig:rgb-order.header-text')}>
						<p className="card-text">
							{t('LedConfig:rgb-order.sub-header-text')}
						</p>
						<p className="card-text">
							{t('LedConfig:rgb-order.sub-header1-text')}
						</p>
						<DraggableListGroup
							groupName="test"
							titles={[
								t('LedConfig:rgb-order.available-header-text'),
								t('LedConfig:rgb-order.assigned-header-text'),
							]}
							dataSources={dataSources}
							onChange={(a) =>
								ledOrderChanged(setFieldValue, a, values.ledsPerButton)
							}
						/>
					</Section>
					<Section title={t('LedConfig:player.header-text')}>
						<Form.Group as={Col}>
							<Row>
								<FormSelect
									label={t('LedConfig:player.pled-type-label')}
									name="pledType"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledType}
									error={errors.pledType}
									isInvalid={errors.pledType}
									onChange={(e) =>
										setFieldValue('pledType', parseInt(e.target.value))
									}
								>
									<option value="-1" defaultValue={true}>
										{t('LedConfig:player.pled-type-off')}
									</option>
									<option value="0">
										{t('LedConfig:player.pled-type-pwm')}
									</option>
									<option value="1">
										{t('LedConfig:player.pled-type-rgb')}
									</option>
								</FormSelect>
								<FormControl
									type="number"
									name="pledPin1"
									hidden={parseInt(values.pledType) !== 0}
									label={PLED_LABELS[0][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledPin1}
									error={errors.pledPin1}
									isInvalid={errors.pledPin1}
									onChange={(e) =>
										setFieldValue('pledPin1', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledPin2"
									hidden={parseInt(values.pledType) !== 0}
									label={PLED_LABELS[1][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledPin2}
									error={errors.pledPin2}
									isInvalid={errors.pledPin2}
									onChange={(e) =>
										setFieldValue('pledPin2', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledPin3"
									hidden={parseInt(values.pledType) !== 0}
									label={PLED_LABELS[2][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledPin3}
									error={errors.pledPin3}
									isInvalid={errors.pledPin3}
									onChange={(e) =>
										setFieldValue('pledPin3', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledPin4"
									hidden={parseInt(values.pledType) !== 0}
									label={PLED_LABELS[3][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledPin4}
									error={errors.pledPin4}
									isInvalid={errors.pledPin4}
									onChange={(e) =>
										setFieldValue('pledPin4', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledIndex1"
									hidden={parseInt(values.pledType) !== 1}
									label={PLED_LABELS[0][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledIndex1}
									error={errors.pledIndex1}
									isInvalid={errors.pledIndex1}
									onChange={(e) =>
										setFieldValue('pledIndex1', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledIndex2"
									hidden={parseInt(values.pledType) !== 1}
									label={PLED_LABELS[1][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledIndex2}
									error={errors.pledIndex2}
									isInvalid={errors.pledIndex2}
									onChange={(e) =>
										setFieldValue('pledIndex2', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledIndex3"
									hidden={parseInt(values.pledType) !== 1}
									label={PLED_LABELS[2][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledIndex3}
									error={errors.pledIndex3}
									isInvalid={errors.pledIndex3}
									onChange={(e) =>
										setFieldValue('pledIndex3', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="pledIndex4"
									hidden={parseInt(values.pledType) !== 1}
									label={PLED_LABELS[3][values.pledType]}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.pledIndex4}
									error={errors.pledIndex4}
									isInvalid={errors.pledIndex4}
									onChange={(e) =>
										setFieldValue('pledIndex4', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
                                                                label={t('LedConfig:player.pled-color-label')}
                                                                hidden={parseInt(values.pledType) !== 1}
                                                                name="pledColor"
                                                                className="form-control-sm"
                                                                groupClassName="col-sm-2 mb-3"
                                                                value={values.pledColor}
                                                                error={errors.pledColor}
                                                                isInvalid={errors.pledColor}
                                                                onBlur={handleBlur}
                                                                onClick={(e) => toggleColorPicker('pledColor', e)}
                                                                onChange={(e) => {
                                                                        handleChange(e);
                                                                        setShowPicker(false);
									}}
								/>
                                                                <ColorPicker
                                                                        name={colorPickerName || 'pledColor'}
                                                                        types={[
                                                                                {
                                                                                        value:
                                                                                                (colorPickerName && values[colorPickerName]) ||
                                                                                                values.pledColor,
                                                                                },
                                                                        ]}
                                                                        onChange={(c) => {
                                                                                setFieldValue(colorPickerName || 'pledColor', c);
                                                                                setShowPicker(false);
                                                                        }}
                                                                        onDismiss={() => setShowPicker(false)}
                                                                        placement="top"
                                                                        presetColors={LEDColors.map((c) => ({
                                                                                title: c.name,
                                                                                color: c.value,
                                                                        }))}
                                                                        show={showPicker}
                                                                        target={colorPickerTarget}
                                                                ></ColorPicker>
								<div className="col-sm-3 mb-3">
									<Form.Check
										label={t('LedConfig:turn-off-when-suspended')}
										type="switch"
										name="turnOffWhenSuspended"
										isInvalid={false}
										checked={Boolean(values.turnOffWhenSuspended)}
										onChange={(e) => {
											setFieldValue(
												'turnOffWhenSuspended',
												e.target.checked ? 1 : 0,
											);
										}}
									/>
								</div>
							</Row>
							<p hidden={parseInt(values.pledType) !== 0}>
								{t('LedConfig:player.pwm-sub-header-text')}
							</p>
                                                <p hidden={parseInt(values.pledType) !== 1}>
                                                        <Trans
                                                                ns="LedConfig"
                                                                i18nKey="player.rgb-sub-header-text"
                                                                rgbLedStartIndex={rgbLedStartIndex}
								>
									For RGB LEDs, the indexes must be after the last LED button
									defined in <em>RGB LED Button Order</em> section and likely{' '}
									<strong>starts at index {{ rgbLedStartIndex }}</strong>.
                                                                </Trans>
                                                        </p>
                                                </Form.Group>
                                        </Section>
                                        <Section title={t('LedConfig:grid.header-text')}>
                                                <Form.Group as={Col}>
                                                        <p>{t('LedConfig:grid.sub-header-text')}</p>
                                                        <Row>
                                                                <FormControl
                                                                        label={t('LedConfig:grid.gradient-a')}
                                                                        name="gridGradientColorA"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridGradientColorA}
                                                                        error={errors.gridGradientColorA}
                                                                        isInvalid={errors.gridGradientColorA}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridGradientColorA', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormControl
                                                                        label={t('LedConfig:grid.gradient-b')}
                                                                        name="gridGradientColorB"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridGradientColorB}
                                                                        error={errors.gridGradientColorB}
                                                                        isInvalid={errors.gridGradientColorB}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridGradientColorB', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormControl
                                                                        label={t('LedConfig:grid.button-press')}
                                                                        name="gridButtonPressColor"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridButtonPressColor}
                                                                        error={errors.gridButtonPressColor}
                                                                        isInvalid={errors.gridButtonPressColor}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridButtonPressColor', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormControl
                                                                        label={t('LedConfig:grid.lever-normal')}
                                                                        name="gridLeverNormalColor"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridLeverNormalColor}
                                                                        error={errors.gridLeverNormalColor}
                                                                        isInvalid={errors.gridLeverNormalColor}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridLeverNormalColor', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormControl
                                                                        label={t('LedConfig:grid.lever-press')}
                                                                        name="gridLeverPressColor"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridLeverPressColor}
                                                                        error={errors.gridLeverPressColor}
                                                                        isInvalid={errors.gridLeverPressColor}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridLeverPressColor', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                        </Row>
                                                        <Row>
                                                                <FormControl
                                                                        label={t('LedConfig:grid.case-normal')}
                                                                        name="gridCaseNormalColor"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridCaseNormalColor}
                                                                        error={errors.gridCaseNormalColor}
                                                                        isInvalid={errors.gridCaseNormalColor}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridCaseNormalColor', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormControl
                                                                        label={t('LedConfig:grid.case-press')}
                                                                        name="gridCaseLeverPressColor"
                                                                        className="form-control-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridCaseLeverPressColor}
                                                                        error={errors.gridCaseLeverPressColor}
                                                                        isInvalid={errors.gridCaseLeverPressColor}
                                                                        onBlur={handleBlur}
                                                                        onClick={(e) => toggleColorPicker('gridCaseLeverPressColor', e)}
                                                                        onChange={(e) => {
                                                                                handleChange(e);
                                                                                setShowPicker(false);
                                                                        }}
                                                                />
                                                                <FormSelect
                                                                        label={t('LedConfig:grid.speed-label')}
                                                                        name="gridGradientSpeed"
                                                                        className="form-select-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridGradientSpeed}
                                                                        error={errors.gridGradientSpeed}
                                                                        isInvalid={errors.gridGradientSpeed}
                                                                        onChange={(e) =>
                                                                                setFieldValue(
                                                                                        'gridGradientSpeed',
                                                                                        parseInt(e.target.value),
                                                                                )
                                                                        }
                                                                >
                                                                        {GRID_GRADIENT_SPEED.map((o, i) => (
                                                                                <option key={`grid-speed-${i}`} value={o.value}>
                                                                                        {o.label}
                                                                                </option>
                                                                        ))}
                                                                </FormSelect>
                                                                <FormSelect
                                                                        label={t('LedConfig:grid.pause-label')}
                                                                        name="gridGradientPause"
                                                                        className="form-select-sm"
                                                                        groupClassName="col-sm-2 mb-3"
                                                                        value={values.gridGradientPause}
                                                                        error={errors.gridGradientPause}
                                                                        isInvalid={errors.gridGradientPause}
                                                                        onChange={(e) =>
                                                                                setFieldValue(
                                                                                        'gridGradientPause',
                                                                                        parseInt(e.target.value),
                                                                                )
                                                                        }
                                                                >
                                                                        {GRID_GRADIENT_PAUSE.map((o, i) => (
                                                                                <option key={`grid-pause-${i}`} value={o.value}>
                                                                                        {o.label}
                                                                                </option>
                                                                        ))}
                                                                </FormSelect>
                                                        </Row>
                                                        <p>{t('LedConfig:grid.case-help-text')}</p>
                                                        <Row>
                                                                {[{
                                                                        name: 'gridCaseUpIndices',
                                                                        label: t('LedConfig:grid.case-up-label'),
                                                                },
                                                                {
                                                                        name: 'gridCaseDownIndices',
                                                                        label: t('LedConfig:grid.case-down-label'),
                                                                },
                                                                {
                                                                        name: 'gridCaseRightIndices',
                                                                        label: t('LedConfig:grid.case-right-label'),
                                                                },
                                                                {
                                                                        name: 'gridCaseLeftIndices',
                                                                        label: t('LedConfig:grid.case-left-label'),
                                                                }].map((direction, index) => (
                                                                        <Col sm={6} md={3} key={`grid-case-${index}`} className="mb-3">
                                                                                <Form.Label>{direction.label}</Form.Label>
                                                                                {[0, 1, 2, 3].map((slot) => {
                                                                                        const dirArray = ensureIndexArray(values[direction.name]);
                                                                                        return (
                                                                                                <FormControl
                                                                                                        key={`${direction.name}-${slot}`}
                                                                                                        type="number"
                                                                                                        name={`${direction.name}-${slot}`}
                                                                                                        className="form-control-sm mb-2"
                                                                                                        value={dirArray[slot]}
                                                                                                        min={0}
                                                                                                        onChange={(e) => {
                                                                                                                const updatedArray = ensureIndexArray(
                                                                                                                        values[direction.name],
                                                                                                                );
                                                                                                                updatedArray[slot] = e.target.value;
                                                                                                                setFieldValue(direction.name, updatedArray);
                                                                                                        }}
                                                                                                />
                                                                                        );
                                                                                })}
                                                                        </Col>
                                                                ))}
                                                        </Row>
                                                </Form.Group>
                                        </Section>
                                        <Section title={t('LedConfig:case.header-text')}>
                                                <Form.Group as={Col}>
                                                        <Row>
                                                                <FormSelect
									label={t('LedConfig:case.case-type-label')}
									name="caseRGBType"
									className="form-select-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.caseRGBType}
									error={errors.caseRGBType}
									isInvalid={errors.caseRGBType}
									onChange={(e) =>
										setFieldValue('caseRGBType', parseInt(e.target.value))
									}
								>
									{CASE_TYPE.map((o, i) => (
										<option key={`caseType-option-${i}`} value={o.value}>
											{o.label}
										</option>
									))}
								</FormSelect>
								<FormControl
									type="number"
									name="caseRGBIndex"
									hidden={parseInt(values.caseRGBType) === -1}
									label={t('LedConfig:case.case-index-label')}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.caseRGBIndex}
									error={errors.caseRGBIndex}
									isInvalid={errors.caseRGBIndex}
									onChange={(e) =>
										setFieldValue('caseRGBIndex', parseInt(e.target.value))
									}
									min={0}
								/>
								<FormControl
									type="number"
									name="caseRGBCount"
									hidden={parseInt(values.caseRGBType) === -1}
									label={t('LedConfig:case.case-count-label')}
									className="form-control-sm"
									groupClassName="col-sm-2 mb-3"
									value={values.caseRGBCount}
									error={errors.caseRGBCount}
									isInvalid={errors.caseRGBCount}
									onChange={(e) =>
										setFieldValue('caseRGBCount', parseInt(e.target.value))
									}
									min={0}
								/>
							</Row>
							<p>{t('LedConfig:case.sub-header-text')}</p>
						</Form.Group>
					</Section>
					<Button type="submit">{t('Common:button-save-label')}</Button>
					{saveMessage ? <span className="alert">{saveMessage}</span> : null}
					<FormContext
						{...{
							buttonLabelType,
							ledButtonMap: values.ledButtonMap,
							swapTpShareLabels,
							setDataSources,
						}}
					/>
				</Form>
			)}
		</Formik>
	);
}
