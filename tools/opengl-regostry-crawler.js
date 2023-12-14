const cheerio = require('cheerio')
const axios = require('axios');
const ncp = require('copy-paste');

(async () => {
    const func = 'glGetProgramInfoLog';
    const url = `https://registry.khronos.org/OpenGL-Refpages/gl4/html/${func}.xhtml`;

    const pageHTML = await axios.get(url);

    // initializing cheerio
    const $ = cheerio.load(pageHTML.data, {
        scriptingEnabled: false,
        xml: {
            // Disable `xmlMode` to parse HTML with htmlparser2.
            xmlMode: false,
        },
    });

    let doc = '';
    console.log($);

    doc += '/// \\name ' + $('div.refnamediv > p').text() + '\n';

    let paramsNames = [];
    let paramsDescriptions = [];
    const params = $('#parameters > div > dl');
    params.find('dd > p').each((i, e) => {
        paramsDescriptions.push($(e).text());
    });
    params.find('dt > span > em > code').each((i, e) => {
        paramsNames.push($(e).text());
    });
    console.assert(paramsNames.length === paramsDescriptions.length);
    for (let i = 0; i < paramsNames.length; i++) {
        paramsNames[i] = paramsNames[i].replaceAll('\n', '');
        paramsDescriptions[i] = paramsDescriptions[i].replaceAll('\n', '');
        paramsDescriptions[i] = paramsDescriptions[i].replace(/( )+/g, ' ');
        doc += '/// \\param ' + paramsNames[i] + ' ' + paramsDescriptions[i] + '\n';
    }

    $('#description').find('p').each((i, e) => {
        if (i === 0) doc += '/// \\brief ';
        else doc += '/// ';
        doc += $(e).text().trim().replaceAll('\n', '').replace(/( )+/g, ' ') + '\n';
    });

    console.log(doc);


    ncp.copy(doc);
})()


