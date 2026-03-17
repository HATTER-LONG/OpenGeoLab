.pragma library

function allFields(positionFields, dimensionFields) {
    const fields = [];
    if (positionFields) {
        fields.push(...positionFields);
    }
    if (dimensionFields) {
        fields.push(...dimensionFields);
    }
    return fields;
}

function createInitialFormValues(requestSpec, positionFields, dimensionFields) {
    const nextValues = {
        "modelName": requestSpec && requestSpec.defaultName ? requestSpec.defaultName : ""
    };
    for (const field of allFields(positionFields, dimensionFields)) {
        nextValues[field.key] = field.defaultValue;
    }
    return nextValues;
}

function defaultAxis(requestSpec) {
    return requestSpec && requestSpec.defaultAxis ? requestSpec.defaultAxis : "Z";
}

function fieldValue(formValues, fieldKey) {
    if (!formValues || formValues[fieldKey] === undefined || formValues[fieldKey] === null) {
        return "";
    }
    return String(formValues[fieldKey]);
}

function withFieldValue(formValues, fieldKey, nextValue) {
    const nextValues = Object.assign({}, formValues || {});
    nextValues[fieldKey] = nextValue;
    return nextValues;
}

function numericValue(formValues, fieldKey, fallbackValue) {
    const parsedValue = Number(fieldValue(formValues, fieldKey));
    return isFinite(parsedValue) ? parsedValue : fallbackValue;
}

function validateField(formValues, field) {
    const rawValue = fieldValue(formValues, field.key).trim();
    const parsedValue = Number(rawValue);
    if (rawValue.length === 0 || !isFinite(parsedValue)) {
        return {
            "success": false,
            "fieldKey": field.key,
            "message": qsTr("Enter a valid number for %1.").arg(field.label)
        };
    }
    if (field.positiveOnly && parsedValue <= 0) {
        return {
            "success": false,
            "fieldKey": field.key,
            "message": qsTr("%1 must be greater than zero.").arg(field.label)
        };
    }
    return {
        "success": true,
        "value": parsedValue
    };
}

function assignPath(target, path, value) {
    let current = target;
    for (let index = 0; index < path.length - 1; ++index) {
        const segment = path[index];
        if (current[segment] === undefined || current[segment] === null || typeof current[segment] !== "object") {
            current[segment] = {};
        }
        current = current[segment];
    }
    current[path[path.length - 1]] = value;
}

function validateShapeSpecific(requestSpec, param) {
    if (!requestSpec) {
        return {
            "success": false,
            "fieldKey": "",
            "message": qsTr("Geometry create request is not configured.")
        };
    }

    if (requestSpec.shapeType === "torus" && param.minorRadius >= param.majorRadius) {
        return {
            "success": false,
            "fieldKey": "minorRadius",
            "message": qsTr("Minor radius must be smaller than major radius.")
        };
    }

    return {
        "success": true
    };
}

function buildValidatedRequest(input) {
    if (!input.requestSpec) {
        return {
            "success": false,
            "fieldKey": "",
            "message": qsTr("Geometry create request is not configured.")
        };
    }

    const trimmedModelName = fieldValue(input.formValues, "modelName").trim();
    const param = {
        "modelName": trimmedModelName.length > 0 ? trimmedModelName : input.requestSpec.defaultName,
        "source": input.requestSource
    };

    for (const field of allFields(input.positionFields, input.dimensionFields)) {
        const fieldResult = validateField(input.formValues, field);
        if (!fieldResult.success) {
            return fieldResult;
        }
        assignPath(param, field.path || [field.key], fieldResult.value);
    }

    if (input.supportsAxis) {
        param.axis = input.axisValue;
    }

    const shapeValidation = validateShapeSpecific(input.requestSpec, param);
    if (!shapeValidation.success) {
        return shapeValidation;
    }

    return {
        "success": true,
        "request": {
            "module": input.requestSpec.module,
            "action": input.requestSpec.action,
            "param": param
        }
    };
}

function requestJson(input) {
    const result = buildValidatedRequest(input);
    return result.success ? JSON.stringify(result.request) : "";
}

function advisoryMessage(requestSpec, formValues) {
    if (!requestSpec || requestSpec.shapeType !== "torus") {
        return "";
    }

    const majorRadius = numericValue(formValues, "majorRadius", NaN);
    const minorRadius = numericValue(formValues, "minorRadius", NaN);
    if (!isFinite(majorRadius) || !isFinite(minorRadius)) {
        return "";
    }

    return minorRadius >= majorRadius
        ? qsTr("Minor radius should stay below major radius for a valid torus.")
        : "";
}

function formatNumber(value) {
    return isFinite(value) ? value.toFixed(3) : "0.000";
}

function derivedMetrics(requestSpec, formValues) {
    if (!requestSpec) {
        return [];
    }

    if (requestSpec.shapeType === "box") {
        const sizeX = numericValue(formValues, "sizeX", 0.0);
        const sizeY = numericValue(formValues, "sizeY", 0.0);
        const sizeZ = numericValue(formValues, "sizeZ", 0.0);
        const diagonal = Math.sqrt(sizeX * sizeX + sizeY * sizeY + sizeZ * sizeZ);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber(sizeX * sizeY * sizeZ) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Diagonal"),
                "value": formatNumber(diagonal) + " mm",
                "accent": "accentE"
            }
        ];
    }

    if (requestSpec.shapeType === "cylinder") {
        const radius = numericValue(formValues, "radius", 0.0);
        const height = numericValue(formValues, "height", 0.0);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber(Math.PI * radius * radius * height) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Surface Area"),
                "value": formatNumber(2 * Math.PI * radius * (radius + height)) + " mm²",
                "accent": "accentC"
            }
        ];
    }

    if (requestSpec.shapeType === "sphere") {
        const radius = numericValue(formValues, "radius", 0.0);
        return [
            {
                "label": qsTr("Volume"),
                "value": formatNumber((4.0 / 3.0) * Math.PI * Math.pow(radius, 3)) + " mm³",
                "accent": "accentA"
            },
            {
                "label": qsTr("Surface Area"),
                "value": formatNumber(4.0 * Math.PI * radius * radius) + " mm²",
                "accent": "accentC"
            },
            {
                "label": qsTr("Diameter"),
                "value": formatNumber(radius * 2.0) + " mm",
                "accent": "accentE"
            }
        ];
    }

    const majorRadius = numericValue(formValues, "majorRadius", 0.0);
    const minorRadius = numericValue(formValues, "minorRadius", 0.0);
    return [
        {
            "label": qsTr("Volume"),
            "value": formatNumber(2 * Math.PI * Math.PI * majorRadius * minorRadius * minorRadius) + " mm³",
            "accent": "accentA"
        },
        {
            "label": qsTr("Surface Area"),
            "value": formatNumber(4 * Math.PI * Math.PI * majorRadius * minorRadius) + " mm²",
            "accent": "accentC"
        },
        {
            "label": qsTr("Outer Diameter"),
            "value": formatNumber((majorRadius + minorRadius) * 2.0) + " mm",
            "accent": "accentE"
        }
    ];
}
