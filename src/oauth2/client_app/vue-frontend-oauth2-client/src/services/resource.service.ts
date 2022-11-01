interface ResourceResponse {
    content: string
}

class ResourceService {
    private readonly apiUrl: string = 'http://localhost:8083/access'

    async accessResource(accessToken: string, clientId: string): Promise<ResourceResponse> {
        const response = await fetch(
            `${this.apiUrl}?access_token=${accessToken}&client_id=${clientId}`,
            {
                method: 'get',
                mode: 'cors',
                headers: {
                    'Origin': 'http://localhost:8081'
                },
            })
        if (response.status === 200) {
            return response.json()
        }
        throw new Error(`Resource server responded with status code ${response.status}: ${await response.text()}`)
    }
}

export const resourceService = new ResourceService()
